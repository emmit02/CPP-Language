#include <fstream>
#include <sstream>
#include <map>
#include <string>
#include <cmath>
#include <ctime>
#include <iomanip>
#include <chrono>
#include <thread>
#include <vector>
#include <unordered_set>
#include <mutex>
#include <unordered_map>
#include <utility>

#include <queue>
#include <condition_variable>
#include <iostream>

using namespace std;
// Struct to hold parsed data
struct ParsedData {
    uint64_t epoch_time;
    int token;
    double price;
    char order_traded_type;
    uint64_t exchange_ordernumber1;
    uint64_t exchange_ordernumber2;
    char buysellflag;
};

// Shared resources for producer-consumer
std::queue<ParsedData> dataQueue;
std::mutex queueMutex;
std::condition_variable dataAvailable;
bool stopProcessing = false; // To signal threads to stop

#define COLOR_GREEN "\033[32m"
#define COLOR_RED "\033[31m"
#define COLOR_RESET "\033[0m"

struct OHLCData {
    double open = 0.0;
    double high = -INFINITY;
    double low = INFINITY;
    double close = 0.0;
    double volume = 0.0;
};

struct StickData {
    uint64_t time; // For comparison, assuming time is an integer
};

// Custom hash function for pairs
namespace std {
    template <typename T1, typename T2>
    struct hash<std::pair<T1, T2>> {
        size_t operator()(const std::pair<T1, T2>& p) const {
            // Combine the hash values of the two elements of the pair
            auto h1 = std::hash<T1>{}(p.first);
            auto h2 = std::hash<T2>{}(p.second);
            
            // Combine the two hashes using bitwise XOR and shifting
            return h1 ^ (h2 << 1); 
        }
    };
}

std::unordered_map<int, OHLCData> currentOHLC; // Token-specific OHLC data

std::unordered_map<uint64_t, StickData> bsstick; // Stick data
// Cache to store comparison results between order numbers
std::unordered_map<std::pair<uint64_t, uint64_t>, char, std::hash<std::pair<uint64_t, uint64_t>>> comparisonCache;

std::mutex ohlcMutex; // Mutex for thread safety
std::mutex bsstickMutex; // Mutex for stick map
std::condition_variable dataReady; // Notify threads for new data

std::unordered_map<int, std::vector<std::tuple<double, uint64_t, uint64_t, uint64_t>>> tokenDataQueue; // Token-specific data queue
bool processing = true; // Control flag

void print_ohlc(int token, const OHLCData &ohlc, uint64_t epoch_second, uint64_t exchange_ordernumber1, uint64_t exchange_ordernumber2) {
    // Convert epoch_second to human-readable time
    std::time_t seconds = static_cast<std::time_t>(epoch_second);
    std::tm *time_info = std::localtime(&seconds);
    std::ostringstream time_stream;
    time_stream << std::put_time(time_info, "%H:%M:%S");

    // Check the cache first for the comparison result
    auto cache_key = std::make_pair(exchange_ordernumber1, exchange_ordernumber2);
    char stick_type = '\0';
    auto cache_it = comparisonCache.find(cache_key);
    
    if (cache_it != comparisonCache.end()) {
        // Cache hit, directly use cached result
        stick_type = cache_it->second;
    } else {
        // Cache miss, perform lookup in the main map
        auto it1 = bsstick.find(exchange_ordernumber1);
        auto it2 = bsstick.find(exchange_ordernumber2);

        if (it1 != bsstick.end() && it2 != bsstick.end()) {
            // Compare timestamps to determine stick type
            stick_type = (it1->second.time > it2->second.time) ? 'B' : 'S';
            
            // Cache the comparison result for future lookups
             comparisonCache[cache_key] = stick_type;
        }
    }

    if (stick_type != '\0') {
        // Efficiently construct the output using ostringstream
        std::ostringstream output;
        output << "Token: " << token
               << " | Time: [" << time_stream.str() << "]"
               << " | Open: " << ohlc.open
               << " | High: " << ohlc.high
               << " | Low: " << ohlc.low
               << " | Close: " << ohlc.close
               << " | Volume: " << ohlc.volume
               << " | StickType: " << stick_type;

        // Output the final string
        std::cout << output.str() << std::endl;
    }
}

uint64_t currentSecond = 0;
std::mutex mtx;  // Mutex for thread safety

int line_counter = 0; 
void process_token_data(int token, double price, uint64_t epoch_microseconds, uint64_t exchange_ordernumber1, uint64_t exchange_ordernumber2) {
    std::lock_guard<std::mutex> lock(mtx); // Lock mutex to ensure thread safety
    
    uint64_t epoch_second = epoch_microseconds / 1000000;

    // Debug print for epoch time to check conversion correctness
    //std::cout << "Epoch Time (Seconds): " << epoch_second << std::endl;

    // Check if we've moved to a new second
    if (epoch_second != currentSecond) {
        // Print accumulated OHLC data for all tokens if moving to a new second
        for (auto &[tkn, ohlc] : currentOHLC) {
            // Debug print for each token's OHLC data before printing
            // std::cout << "Token: " << tkn << ", OHLC: " 
            //           << ohlc.open << ", " << ohlc.high << ", " 
            //           << ohlc.low << ", " << ohlc.close << ", Volume: " 
            //           << ohlc.volume << std::endl;

            print_ohlc(tkn, ohlc, currentSecond, exchange_ordernumber1, exchange_ordernumber2);
        }
        currentOHLC.clear();  // Clear data for the next second
        currentSecond = epoch_second;  // Update the current second
    }

    // Update OHLC data for the current token
    auto &ohlc = currentOHLC[token];

    // Initialize the OHLC for the first data point (only if price is valid)
    if (ohlc.open == 0.0 && price != 0.0) {
        ohlc.open = price;
        ohlc.high = price;
        ohlc.low = price;
    } else {
        // Update high/low based on new price
        ohlc.high = std::max(ohlc.high, price);
        ohlc.low = std::min(ohlc.low, price);
    }

    // Set the close price to the current price
    ohlc.close = price;

    // Accumulate the volume (assuming quantity is passed and it's a valid value)
    // You need to update the function to pass the 'quantity' if available, otherwise, volume will accumulate price.
    // Example: if you have 'quantity', then use that.
    // If no quantity is available, and volume should be price-based, retain it.
    ohlc.volume += price;  // Accumulate trade volume (this might be incorrect if volume needs quantity)
    
    // Debug print for each token's OHLC data after update
    // std::cout << "Updated Token: " << token << ", OHLC: " 
    //           << ohlc.open << ", " << ohlc.high << ", " 
    //           << ohlc.low << ", " << ohlc.close << ", Volume: " 
    //           << ohlc.volume << std::endl;

    line_counter++;  // Increment line counter for tracking
}



bool parse_and_enqueue(const std::string &line) {
    ParsedData parsedData;
    std::stringstream ss(line);
    std::string temp;

    try {
        // Parse fields from the line
        std::getline(ss, temp, ',');
        std::getline(ss, temp, ',');
        try
        {
            parsedData.epoch_time = std::stoull(temp);
        }
        catch (...)
        {
            return false; // Handle invalid parsing
        }
        std::getline(ss, temp, ',');
        std::getline(ss, temp, ',');
        std::getline(ss, temp, ',');
        std::getline(ss, temp, ',');
        parsedData.order_traded_type = temp[0];
        std::getline(ss, temp, ',');
        std::getline(ss, temp, ',');
        try
        {
            parsedData.exchange_ordernumber1 = std::stoull(temp);
        }
        catch (...)
        {
            return false; // Handle invalid parsing
        }

        std::getline(ss, temp, ',');
        if (parsedData.order_traded_type == 'T')
        {
            try
            {
                parsedData.exchange_ordernumber2 = std::stoull(temp);
            }
            catch (...)
            {
                return false; // Handle invalid parsing
            }
        }
        std::getline(ss, temp, ',');
        
        if (parsedData.order_traded_type == 'T')
        {
            try
            {
                parsedData.token = std::stoi(temp);
            }
            catch (...)
            {
                return false; // Handle invalid parsing
            }
        }
        if (parsedData.order_traded_type == 'M' || parsedData.order_traded_type == 'N')
        {
            parsedData.buysellflag= temp[0];
        }

        std::getline(ss, temp, ',');
        try
        {
            parsedData.price = std::stod(temp) / 100.0; // Convert to rupees
        }
        catch (...)
        {
            return false; // Handle invalid parsing
        }
        // Enqueue parsed data
        {
            std::lock_guard<std::mutex> lock(queueMutex);
            dataQueue.push(parsedData);
        }
        dataAvailable.notify_one(); // Notify a worker thread

        return true;
    } catch (...) {
        return false; // Handle invalid parsing
    }
}

void update_ohlc(const ParsedData &data) {
    process_token_data(data.token, data.price, data.epoch_time, data.exchange_ordernumber1, data.exchange_ordernumber2);
}

void update_bsstick(const ParsedData &data) {
    std::lock_guard<std::mutex> lock(queueMutex);
    bsstick[data.exchange_ordernumber1] = {data.epoch_time};
}

void worker_thread() {
    while (true) {
        ParsedData data;

        {
            std::unique_lock<std::mutex> lock(queueMutex);
            dataAvailable.wait(lock, [] { return !dataQueue.empty() || stopProcessing; });

            if (stopProcessing && dataQueue.empty())
                break;

            data = dataQueue.front();
            dataQueue.pop();
        }

        if (data.order_traded_type == 'T' && data.token == 25) {
            update_ohlc(data);
        } else {
            update_bsstick(data);
        }
    }
}

void process_file(const std::string &file_path) {
    std::ifstream file(file_path);
    if (!file.is_open()) {
        std::cerr << "Error opening file" << std::endl;
        return;
    }

    // Move to the end of the file
    file.seekg(0, std::ios::end);
    
    while (true) {
        std::string line;
        // Read new data that gets appended to the file
        if (std::getline(file, line)) {
            if (parse_and_enqueue(line)) {
                // Process the line
            } else {
                std::cerr << "Failed to parse line: " << line << std::endl;
            }
        } else {
            // Wait before trying again (this is useful for monitoring growing files)
            std::this_thread::sleep_for(std::chrono::milliseconds(100));  // Adjust the sleep time
            file.clear();  // Clear EOF flag to try again
        }
    }

    // Notify workers to stop after processing all data
    {
        std::lock_guard<std::mutex> lock(queueMutex);
        stopProcessing = true;
    }
    dataAvailable.notify_all();
}


int main(int argc, char *argv[]) {
    if (argc < 2) {
        std::cerr << "Usage: " << argv[0] << " <file_path>" << std::endl;
        return 1;
    }

    // Start worker threads
    const int numWorkers = 4; // Adjust based on available hardware
    std::vector<std::thread> workers;
    for (int i = 0; i < numWorkers; ++i) {
        workers.emplace_back(worker_thread);
    }

    // Start producer
    process_file(argv[1]);

    // Wait for all workers to finish
    for (auto &worker : workers) {
        worker.join();
    }

    return 0;
}
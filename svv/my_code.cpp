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
#include <random>

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

struct OHLCData {
    double open = 0.0;
    double high = -INFINITY;
    double low = INFINITY;
    double close = 0.0;
    double volume = 0.0;
};

// Custom hash function for pairs
namespace std {
    template <typename T1, typename T2>
    struct hash<std::pair<T1, T2>> {
        size_t operator()(const std::pair<T1, T2>& p) const {
            auto h1 = std::hash<T1>{}(p.first);
            auto h2 = std::hash<T2>{}(p.second);
            return h1 ^ (h2 << 1);
        }
    };
}

std::unordered_map<int, OHLCData> currentOHLC; // Token-specific OHLC data
std::unordered_map<uint64_t, uint64_t> bsstick; // Stick data (timestamp mapping)
std::unordered_map<std::pair<uint64_t, uint64_t>, char, std::hash<std::pair<uint64_t, uint64_t>>> comparisonCache; // Cache for comparisons

std::mutex ohlcMutex; // Mutex for thread safety
uint64_t currentSecond = 0; // Track current epoch second

void print_ohlc(int token, const OHLCData &ohlc, uint64_t epoch_second) {
    std::time_t seconds = static_cast<std::time_t>(epoch_second);
    std::tm *time_info = std::localtime(&seconds);
    std::ostringstream time_stream;
    time_stream << std::put_time(time_info, "%H:%M:%S");

    std::cout << "Token: " << token
              << " | Time: [" << time_stream.str() << "]"
              << " | Open: " << ohlc.open
              << " | High: " << ohlc.high
              << " | Low: " << ohlc.low
              << " | Close: " << ohlc.close
              << " | Volume: " << ohlc.volume << std::endl;
}

void process_token_data(int token, double price, uint64_t epoch_microseconds) {
    std::lock_guard<std::mutex> lock(ohlcMutex);
    uint64_t epoch_second = epoch_microseconds / 1000000;

    if (epoch_second != currentSecond) {
        for (auto &[tkn, ohlc] : currentOHLC) {
            print_ohlc(tkn, ohlc, currentSecond);
        }
        currentOHLC.clear();
        currentSecond = epoch_second;
    }

    auto &ohlc = currentOHLC[token];
    if (ohlc.open == 0.0) {
        ohlc.open = ohlc.high = ohlc.low = price;
    } else {
        ohlc.high = std::max(ohlc.high, price);
        ohlc.low = std::min(ohlc.low, price);
    }
    ohlc.close = price;
    ohlc.volume += price;
}

void update_ohlc(const ParsedData &data) {
    process_token_data(data.token, data.price, data.epoch_time);
}

void update_bsstick(const ParsedData &data) {
    std::lock_guard<std::mutex> lock(queueMutex);
    bsstick[data.exchange_ordernumber1] = data.epoch_time;
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

        if (data.order_traded_type == 'T') {
            update_ohlc(data);
        } else {
            update_bsstick(data);
        }
    }
}

void generate_random_data() {
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<uint64_t> epoch_dist(1609459200000000, 1609462800000000);
    std::uniform_int_distribution<int> token_dist(1, 100);
    std::uniform_real_distribution<double> price_dist(100.0, 1000.0);
    std::uniform_int_distribution<uint64_t> ordernumber_dist(1, 1000000);
    std::uniform_int_distribution<int> type_dist(0, 1);
    std::uniform_int_distribution<int> flag_dist(0, 1);

    while (!stopProcessing) {
        ParsedData data;

        data.epoch_time = epoch_dist(gen);
        data.token = token_dist(gen);
        data.price = price_dist(gen);
        data.exchange_ordernumber1 = ordernumber_dist(gen);
        data.exchange_ordernumber2 = ordernumber_dist(gen);
        data.order_traded_type = type_dist(gen) == 0 ? 'T' : 'M';
        if (data.order_traded_type == 'M') {
            data.buysellflag = flag_dist(gen) == 0 ? 'B' : 'S';
        }

        {
            std::lock_guard<std::mutex> lock(queueMutex);
            dataQueue.push(data);
        }
        dataAvailable.notify_one();

        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
}

int main() {
    const int numWorkers = 4;
    std::vector<std::thread> workers;
    for (int i = 0; i < numWorkers; ++i) {
        workers.emplace_back(worker_thread);
    }

    std::thread producer(generate_random_data);

    std::this_thread::sleep_for(std::chrono::seconds(10)); // Run for 10 seconds
    {
        std::lock_guard<std::mutex> lock(queueMutex);
        stopProcessing = true;
    }
    dataAvailable.notify_all();

    for (auto &worker : workers) {
        worker.join();
    }
    producer.join();

    return 0;
}

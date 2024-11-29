#include <iostream>
#include <fstream>
#include <sstream>
#include <map>
#include <string>
#include <cmath>
#include <ctime>
#include <iomanip>
#include <thread>
#include <chrono>

#ifdef _WIN32
#include <windows.h> // For Sleep() and console color handling on Windows
#endif
    
using namespace std;

#define COLOR_GREEN "\033[32m"
#define COLOR_RED "\033[31m"
#define COLOR_RESET "\033[0m"

struct OHLCData
{
    double open = 0.0;
    double high = -INFINITY;
    double low = INFINITY;
    double close = 0.0;
    double volume = 0.0;
};
struct CompositeValue
{
    char character;
    uint64_t time;
};

map<uint64_t, CompositeValue> bsstick;
std::map<int, OHLCData> currentOHLC; // Store current second's OHLC data for each token
uint64_t currentSecond = 0;          // Track the current second being processed
int count = 1;

#ifdef _WIN32
void set_console_color(int color_code)
{
    // Color codes are mapped to console attributes in Windows
    HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
    SetConsoleTextAttribute(hConsole, color_code);
}
#else
// Linux/Unix colors work by default with ANSI escape codes
#endif

void print_ohlc(int token, const OHLCData &ohlc, uint64_t epoch_second, uint64_t exchange_ordernumber1, uint64_t exchange_ordernumber2, char order_traded_type)
{
    // Convert epoch_second to human-readable time
    std::time_t seconds = static_cast<time_t>(epoch_second);
    std::tm *time_info = std::localtime(&seconds);
    std::ostringstream time_stream;
    time_stream << std::put_time(time_info, "%H:%M:%S");

    if (order_traded_type == 'T')
    {
        cout << "  | "
             << std::setw(6) << "Token:" << std::setw(10) << token
             << " | "
             << std::setw(6) << "Time:" << std::setw(10) << "[" << time_stream.str() << "]"
             << " | "
             << std::setw(6) << "Open:" << std::setw(10) << std::fixed << std::setprecision(2) << ohlc.open
             << " | "
             << std::setw(6) << "High:" << std::setw(10) << std::fixed << std::setprecision(2) << ohlc.high
             << " | "
             << std::setw(6) << "Low:" << std::setw(10) << std::fixed << std::setprecision(2) << ohlc.low
             << " | "
             << std::setw(6) << "Close:" << std::setw(10) << std::fixed << std::setprecision(2) << ohlc.close
             << " | "
             << std::setw(6) << "Volume:" << std::setw(10) << std::fixed << std::setprecision(2) << ohlc.volume
             << " | "
             << std::endl;
    }
}

void process_token_data(int token, double price, uint64_t epoch_microseconds, uint64_t exchange_ordernumber1, uint64_t exchange_ordernumber2, char order_traded_type)
{
    uint64_t epoch_second = epoch_microseconds / 1000000;

    if (epoch_second != currentSecond)
    {
        // Print accumulated OHLC data for all tokens if moving to a new second
        for (const auto &[tkn, ohlc] : currentOHLC)
        {
            print_ohlc(tkn, ohlc, currentSecond, exchange_ordernumber1, exchange_ordernumber2, order_traded_type);
        }
        // Reset for the new second
        currentOHLC.clear();
        currentSecond = epoch_second;
    }

    // Update OHLC data for the current token
    auto &ohlc = currentOHLC[token];
    if (ohlc.open == 0.0)
    { // First entry for this token
        ohlc.open = price;
        ohlc.high = price;
        ohlc.low = price;
    }
    else
    {
        ohlc.high = std::max(ohlc.high, price);
        ohlc.low = std::min(ohlc.low, price);
    }
    ohlc.close = price;
    ohlc.volume += price;

    count++;
}

bool parse_line(const std::string &line, uint64_t &epoch_time, int &token, double &price, char &order_traded_type, uint64_t &exchange_ordernumber1, uint64_t &exchange_ordernumber2, char &buysellflag)
{
    std::stringstream ss(line);
    std::string temp;

    // Parse fields from the line
    std::getline(ss, temp, ',');
    std::getline(ss, temp, ',');
    try
    {
        epoch_time = std::stoull(temp);
    }
    catch (...)
    {
        return false; // Handle invalid parsing
    }
    std::getline(ss, temp, ',');
    std::getline(ss, temp, ',');
    std::getline(ss, temp, ',');
    std::getline(ss, temp, ',');
    order_traded_type = temp[0];
    std::getline(ss, temp, ',');
    std::getline(ss, temp, ',');
    try
    {
        exchange_ordernumber1 = std::stoull(temp);
    }
    catch (...)
    {
        return false; // Handle invalid parsing
    }

    std::getline(ss, temp, ',');
    if (order_traded_type == 'T')
    {
        try
        {
            exchange_ordernumber2 = std::stoull(temp);
        }
        catch (...)
        {
            return false; // Handle invalid parsing
        }
    }
    std::getline(ss, temp, ',');

    if (order_traded_type == 'T')
    {
        try
        {
            token = std::stoi(temp);
        }
        catch (...)
        {
            return false; // Handle invalid parsing
        }
    }
    if (order_traded_type == 'M' || order_traded_type == 'N')
    {
        buysellflag = temp[0];
    }

    std::getline(ss, temp, ',');
    try
    {
        price = std::stod(temp) / 100.0; // Convert to rupees
    }
    catch (...)
    {
        return false; // Handle invalid parsing
    }
    return true;
}

void process_file(const std::string &file_path) {
    std::ifstream file(file_path);
    if (!file.is_open()) {
        std::cerr << "Error opening file" << std::endl;
        return;
    }

    std::string line;
    uint64_t epoch_time;
    int token;
    double price;
    char order_traded_type;
    uint64_t exchange_ordernumber1;
    uint64_t exchange_ordernumber2;
    char buysellflag;

    file.seekg(0, std::ios::end); // Start at the end for real-time data
    std::streampos last_position = file.tellg();

    while (true) {
        file.clear();              // Clear EOF flag
        file.seekg(last_position); // Start from the last read position

        while (std::getline(file, line)) {
            last_position = file.tellg(); // Update last position
            if (parse_line(line, epoch_time, token, price, order_traded_type, exchange_ordernumber1, exchange_ordernumber2, buysellflag)) {
                if (order_traded_type == 'T' && token == 25) {
                    process_token_data(token, price, epoch_time, exchange_ordernumber1, exchange_ordernumber2, order_traded_type);
                } else if (order_traded_type == 'M' || order_traded_type == 'N') {
                    bsstick[exchange_ordernumber1] = {buysellflag, epoch_time};
                }
            } else {
                std::cerr << "Failed to parse line: " << line << std::endl;
            }
        }

        // Pause before checking for new data
        std::this_thread::sleep_for(std::chrono::milliseconds(50)); // Standard cross-platform sleep
    }
}

int main(int argc, char *argv[])
{
    if (argc < 2)
    {
        std::cerr << "Usage: " << argv[0] << " <file_path>" << std::endl;
        return 1;
    }

    process_file(argv[1]);
    return 0;
}

#include <iostream>
#include <fstream>
#include <cstdlib>
#include <ctime>
#include <thread>       // For std::this_thread::sleep_for
#include <chrono>       // For std::chrono::milliseconds
#include <iomanip>

void generate_data(const std::string &file_name, int num_lines, int interval_ms) {
    std::ofstream outfile(file_name, std::ios::app); // Append to the file
    if (!outfile.is_open()) {
        std::cerr << "Error opening file for writing" << std::endl;
        return;
    }

    std::srand(std::time(nullptr)); // Seed random number generator

    for (int i = 0; i < num_lines; ++i) {
        // Generate random values
        time_t rawtime = std::time(nullptr);
        std::tm *time_info = std::localtime(&rawtime);

        char time_buffer[15];
        strftime(time_buffer, sizeof(time_buffer), "%Y%m%d%H%M%S", time_info);

        int token = 25;
        double price = 10000.0 + static_cast<double>(std::rand() % 5000); // Random price
        char order_traded_type = 'T';
        uint64_t exchange_ordernumber1 = std::rand() % 10000 + 1000;     // Random order ID
        uint64_t exchange_ordernumber2 = std::rand() % 10000 + 1000;     // Random order ID
        char buysellflag = (std::rand() % 2 == 0) ? 'B' : 'S';           // Random Buy/Sell flag

        // Write to file
        outfile << time_buffer << "," << token << "," << price << "," << order_traded_type
                << "," << exchange_ordernumber1 << "," << exchange_ordernumber2 << ","
                << buysellflag << std::endl;

        std::cout << "Generated: " << time_buffer << "," << token << "," << price
                  << "," << order_traded_type << "," << exchange_ordernumber1 << ","
                  << exchange_ordernumber2 << "," << buysellflag << std::endl;

        // Wait for interval
        std::this_thread::sleep_for(std::chrono::milliseconds(interval_ms)); // Ensure this works now
    }

    outfile.close();
}

int main() {
    std::string file_name = "data.txt"; // Output file
    int num_lines = 100;                // Number of lines to generate
    int interval_ms = 1000;             // Interval between lines (ms)

    generate_data(file_name, num_lines, interval_ms);

    return 0;
}


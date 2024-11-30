#include <iostream>
#include <string>
#include <cstdlib>
#include <ctime>
#include <chrono>
#include <iomanip>
#include <thread>
#include <random>

// Function to get current time in HH:MM:SS format
std::string getCurrentTime() {
    auto now = std::chrono::system_clock::now();
    auto time_t_now = std::chrono::system_clock::to_time_t(now);
    std::tm local_time = *std::localtime(&time_t_now);

    std::ostringstream time_stream;
    time_stream << std::put_time(&local_time, "%H:%M:%S");
    return time_stream.str();
}

// Generate random floating-point number
double generateRandomDouble(double min, double max) {
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_real_distribution<> dis(min, max);
    return dis(gen);
}

// Generate random integer
int generateRandomInt(int min, int max) {
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> dis(min, max);
    return dis(gen);
}

int main() {
    int count = 0;

    std::cout << "Standalone server running...\n";
    while (true) {
        // Generate data
        count++;
        int token = generateRandomInt(10000, 20000);
        double value = generateRandomDouble(850.0, 900.0);
        std::string time = getCurrentTime();

        // Display data
        std::cout << "COUNT: " << count
                  << " | Token: " << token
                  << " | Time: [" << time << "]"
                  << " | Open: " << value
                  << " | High: " << value
                  << " | Low: " << value
                  << " | Close: " << value
                  << "\n";

        // Simulate delay
        std::this_thread::sleep_for(std::chrono::microseconds(1));
    }

    return 0;
}

#include <iostream>
#include <fstream>
#include <string>

int main() 
{
    std::ifstream file("result.txt"); // Open the file using ifstream
    if (!file.is_open()) {
        std::cerr << "Error: Could not open file." << std::endl;
        return 1;
    }

    std::string line;
    std::string previousTime = ""; // Store the previous time to avoid duplicates
    int counter = 1;

    while (std::getline(file, line)) 
    {
        std::string time;
        size_t timeStart = line.find("Time: [");
        if (timeStart != std::string::npos) {
            timeStart += 7; // Move past "Time: ["
            size_t timeEnd = line.find("]", timeStart);
            if (timeEnd != std::string::npos) {
                time = line.substr(timeStart, timeEnd - timeStart);
                
                if (time != previousTime) 
                { 
                    std::cout << "counter : " << counter << " | " << line << std::endl;
                    previousTime = time; 
                    counter++;
                }
            }
        }
    }

    file.close(); // Close the file
    return 0;
}

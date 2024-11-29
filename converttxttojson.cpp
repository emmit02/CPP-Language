#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <map>

int counter = 1;

std::string parseToJSON(const std::string& line) 
{
    std::map<std::string, std::string> keyValuePairs;
    std::stringstream ss(line);
    std::string segment;

    // Parse the line
    while (std::getline(ss, segment, '|')) {
        std::size_t colonPos = segment.find(':');
        if (colonPos != std::string::npos) {
            std::string key = segment.substr(0, colonPos);
            std::string value = segment.substr(colonPos + 1);

            // Trim spaces
            key.erase(0, key.find_first_not_of(" \t"));
            key.erase(key.find_last_not_of(" \t") + 1);
            value.erase(0, value.find_first_not_of(" \t"));
            value.erase(value.find_last_not_of(" \t") + 1);

            keyValuePairs[key] = value;
        }
    }

    std::ostringstream json;
    json << "\n\n counter : " << counter << "\n{\n";
    bool first = true;
    for (const auto& pair : keyValuePairs) {
        if (!first) json << ", \n\n";
        json << "\"" << pair.first << "\": \"" << pair.second << "\"";
        first = false;
    }
    json << "\n}";
    counter++;
    return json.str();
}

int main() 
{
    std::ifstream inputFile("result.txt");
    if (!inputFile.is_open()) {
        std::cerr << "Error: Unable to open file!" << std::endl;
        return 1;
    }

    std::string line;
    std::vector<std::string> jsonObjects;

    // Read file line by line and convert to JSON
    while (std::getline(inputFile, line)) {
        if (!line.empty()) {
            std::string jsonObject = parseToJSON(line);
            jsonObjects.push_back(jsonObject);
        }
    }

    inputFile.close();

    // Print JSON array
    std::cout << "[\n";
    for (size_t i = 0; i < jsonObjects.size(); ++i) {
        std::cout << "  " << jsonObjects[i];
        if (i != jsonObjects.size() - 1) std::cout << ",";
        std::cout << "\n";
    }
    std::cout << "\n]" << std::endl;

    return 0;
}

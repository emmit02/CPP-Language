#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <map>
#include <winsock2.h>
#include <ws2tcpip.h>

#pragma comment(lib, "ws2_32.lib")


int c = 1;
std::string parseToJSON(const std::string& line) {
    std::map<std::string, std::string> keyValuePairs;
    std::stringstream ss(line);
    std::string segment;

    while (std::getline(ss, segment, '|')) 
    {
        std::size_t colonPos = segment.find(':');
        if (colonPos != std::string::npos) 
        {
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
    json << "\ncounter : " << c;
    json << "\n{\n";
    bool first = true;
    for (const auto& pair : keyValuePairs) {
        if (!first) json << ",\n";
        json << "  \"" << pair.first << "\": \"" << pair.second << "\"";
        first = false;
    }
    json << "\n}";
    c++;
    return json.str();
}

std::vector<std::string> readAndParseFile(const std::string& filename) {
    std::ifstream inputFile(filename);
    if (!inputFile.is_open()) {
        std::cerr << "Error: Unable to open file!" << std::endl;
        return {};
    }

    std::string line;
    std::vector<std::string> jsonObjects;

    while (std::getline(inputFile, line)) {
        if (!line.empty()) {
            jsonObjects.push_back(parseToJSON(line));
        }
    }

    inputFile.close();
    return jsonObjects;
}

int main() {
    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        std::cerr << "WSAStartup failed." << std::endl;
        return 1;
    }

    SOCKET serverSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (serverSocket == INVALID_SOCKET) {
        std::cerr << "Socket creation failed." << std::endl;
        WSACleanup();
        return 1;
    }

    sockaddr_in serverAddr{};
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(8080);
    serverAddr.sin_addr.s_addr = INADDR_ANY;

    if (bind(serverSocket, (sockaddr*)&serverAddr, sizeof(serverAddr)) == SOCKET_ERROR) {
        std::cerr << "Bind failed." << std::endl;
        closesocket(serverSocket);
        WSACleanup();
        return 1;
    }

    if (listen(serverSocket, 1) == SOCKET_ERROR) {
        std::cerr << "Listen failed." << std::endl;
        closesocket(serverSocket);
        WSACleanup();
        return 1;
    }

    std::cout << "Server is listening on port 8080..." << std::endl;

    sockaddr_in clientAddr{};
    int clientAddrSize = sizeof(clientAddr);
    SOCKET clientSocket = accept(serverSocket, (sockaddr*)&clientAddr, &clientAddrSize);
    if (clientSocket == INVALID_SOCKET) {
        std::cerr << "Accept failed." << std::endl;
        closesocket(serverSocket);
        WSACleanup();
        return 1;
    }

    std::cout << "Client connected." << std::endl;

    char buffer[1024];
    int bytesReceived = recv(clientSocket, buffer, sizeof(buffer) - 1, 0);
    if (bytesReceived > 0) {
        buffer[bytesReceived] = '\0'; // Null-terminate received message
        std::cout << "Message from client: " << buffer << std::endl;
    }

    std::vector<std::string> jsonObjects = readAndParseFile("result.txt");

    for (const auto& jsonObject : jsonObjects) {
        send(clientSocket, jsonObject.c_str(), jsonObject.length(), 0);
        send(clientSocket, "\n", 1, 0); // Send newline for separation
    }

    closesocket(clientSocket);
    closesocket(serverSocket);
    WSACleanup();
    return 0;
}

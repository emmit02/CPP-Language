#include <iostream>
#include <string>
#include <vector>
#include <sstream>
#include <winsock2.h>
#pragma comment(lib, "ws2_32.lib")

#define SERVER_IP "127.0.0.1"
#define PORT 8080
#define MAX_BUFFER_SIZE 1024

void InitializeWinsock() {
    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        std::cerr << "Winsock initialization failed." << std::endl;
        exit(1);
    }
}

void CleanupWinsock() {
    WSACleanup();
}

std::vector<std::string> split(const std::string &str, char delimiter) {
    std::vector<std::string> tokens;
    std::stringstream ss(str);
    std::string token;

    while (std::getline(ss, token, delimiter)) {
        tokens.push_back(token);
    }

    return tokens;
}

std::string extractField(const std::string &line, const std::string &field) {
    size_t start = line.find(field);
    if (start == std::string::npos)
        return "";

    start += field.length() + 2; // Skip the "field: " part
    size_t end = line.find('|', start);
    if (end == std::string::npos)
        end = line.length();

    return line.substr(start, end - start);
}

int main() {
    InitializeWinsock();

    SOCKET clientSocket;
    struct sockaddr_in serverAddr;
    char buffer[MAX_BUFFER_SIZE];

    clientSocket = socket(AF_INET, SOCK_STREAM, 0);
    if (clientSocket == INVALID_SOCKET) {
        std::cerr << "Socket creation failed." << std::endl;
        CleanupWinsock();
        return -1;
    }

    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(PORT);
    serverAddr.sin_addr.s_addr = inet_addr(SERVER_IP);

    if (connect(clientSocket, (struct sockaddr *)&serverAddr, sizeof(serverAddr)) == SOCKET_ERROR) {
        std::cerr << "Connection failed." << std::endl;
        closesocket(clientSocket);
        CleanupWinsock();
        return -1;
    }

    std::cout << "Connected to server. Receiving data..." << std::endl;

    std::vector<std::string> jsonObjects;
    int bytesReceived;
    int counter = 1;  // Counter variable to track JSON objects

    while ((bytesReceived = recv(clientSocket, buffer, MAX_BUFFER_SIZE, 0)) > 0) {
        buffer[bytesReceived] = '\0'; // Null-terminate string
        std::string line(buffer);
        std::string token = extractField(line, "Token");
        std::string close = extractField(line, "Close");
        std::string high = extractField(line, "High");
        std::string low = extractField(line, "Low");
        std::string open = extractField(line, "Open");

        if (!token.empty() && !close.empty() && !high.empty() && !low.empty() && !open.empty()) {
            std::string json = "  {\n"
                               "    \"Counter\": " + std::to_string(counter) + ",\n"
                               "    \"Token\": \"" + token + "\",\n"
                               "    \"Close\": \"" + close + "\",\n"
                               "    \"High\": \"" + high + "\",\n"
                               "    \"Low\": \"" + low + "\",\n"
                               "    \"Open\": \"" + open + "\"\n"
                               "  }";
            jsonObjects.push_back(json);
            ++counter;  // Increment counter
        }
    }

    // Print the JSON array
    std::cout << "[\n";
    for (size_t i = 0; i < jsonObjects.size(); ++i) {
        std::cout << jsonObjects[i];
        if (i < jsonObjects.size() - 1)
            std::cout << ",";
        std::cout << "\n";
    }
    std::cout << "]" << std::endl;

    closesocket(clientSocket);
    CleanupWinsock();

    return 0;
}

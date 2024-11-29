#include <iostream>
#include <winsock2.h>
#pragma comment(lib, "ws2_32.lib")

#define SERVER_IP "127.0.0.1"
#define PORT 8080
#define MAX_BUFFER_SIZE 4096  // Larger buffer for faster data handling

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

void parseAndPrintJSON(const char* data, int& counter) {
    // Use a static buffer for JSON output
    static char jsonBuffer[512];
    
    // Extract fields using manual parsing
    const char* tokenStart = strstr(data, "Token: ");
    const char* closeStart = strstr(data, "Close: ");
    const char* highStart = strstr(data, "High: ");
    const char* lowStart = strstr(data, "Low: ");
    const char* openStart = strstr(data, "Open: ");

    if (tokenStart && closeStart && highStart && lowStart && openStart) {
        // Extract values directly
        tokenStart += 7;  // Skip "Token: "
        closeStart += 7;  // Skip "Close: "
        highStart += 6;   // Skip "High: "
        lowStart += 5;    // Skip "Low: "
        openStart += 6;   // Skip "Open: "

        const char* tokenEnd = strchr(tokenStart, ' ');
        const char* closeEnd = strchr(closeStart, ' ');
        const char* highEnd = strchr(highStart, ' ');
        const char* lowEnd = strchr(lowStart, ' ');
        const char* openEnd = strchr(openStart, ' ');

        // Format JSON object directly into the static buffer
        int jsonLength = snprintf(
            jsonBuffer, sizeof(jsonBuffer),
            "  {\n"
            "    \"Counter\": %d,\n"
            "    \"Token\": \"%.*s\",\n"
            "    \"Close\": \"%.*s\",\n"
            "    \"High\": \"%.*s\",\n"
            "    \"Low\": \"%.*s\",\n"
            "    \"Open\": \"%.*s\"\n"
            "  }",
            counter++,
            tokenEnd ? tokenEnd - tokenStart : 0, tokenStart,
            closeEnd ? closeEnd - closeStart : 0, closeStart,
            highEnd ? highEnd - highStart : 0, highStart,
            lowEnd ? lowEnd - lowStart : 0, lowStart,
            openEnd ? openEnd - openStart : 0, openStart);

        // Print the JSON object
        std::cout.write(jsonBuffer, jsonLength);
        std::cout << ",\n"; // Add a comma for JSON formatting
    }
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

    if (connect(clientSocket, (struct sockaddr*)&serverAddr, sizeof(serverAddr)) == SOCKET_ERROR) {
        std::cerr << "Connection failed." << std::endl;
        closesocket(clientSocket);
        CleanupWinsock();
        return -1;
    }

    std::cout << "Connected to server. Receiving data..." << std::endl;
    std::cout << "[\n";

    int bytesReceived;
    int counter = 1;  // Counter variable for JSON objects

    while ((bytesReceived = recv(clientSocket, buffer, MAX_BUFFER_SIZE, 0)) > 0) {
        buffer[bytesReceived] = '\0'; // Null-terminate buffer for safety

        // Process received data for each line
        char* line = strtok(buffer, "\n");  // Split into lines
        while (line) {
            parseAndPrintJSON(line, counter);  // Parse and print each line as JSON
            line = strtok(NULL, "\n");
        }
    }

    std::cout << "{}\n"; // Close JSON array with an empty object for proper JSON syntax
    std::cout << "]" << std::endl;

    closesocket(clientSocket);
    CleanupWinsock();

    return 0;
}

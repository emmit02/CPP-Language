#include <iostream>
#include <fstream>
#include <string>
#include <winsock2.h>
#pragma comment(lib, "ws2_32.lib")

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

int main() {
    InitializeWinsock();

    SOCKET serverSocket, clientSocket;
    struct sockaddr_in serverAddr, clientAddr;
    int clientAddrSize = sizeof(clientAddr);
    char buffer[MAX_BUFFER_SIZE];

    // Create socket
    serverSocket = socket(AF_INET, SOCK_STREAM, 0);
    if (serverSocket == INVALID_SOCKET) {
        std::cerr << "Socket creation failed." << std::endl;
        CleanupWinsock();
        return -1;
    }

    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(PORT);
    serverAddr.sin_addr.s_addr = INADDR_ANY;

    // Bind socket to address
    if (bind(serverSocket, (struct sockaddr *)&serverAddr, sizeof(serverAddr)) == SOCKET_ERROR) {
        std::cerr << "Bind failed." << std::endl;
        closesocket(serverSocket);
        CleanupWinsock();
        return -1;
    }

    // Listen for incoming connections
    if (listen(serverSocket, 1) == SOCKET_ERROR) {
        std::cerr << "Listen failed." << std::endl;
        closesocket(serverSocket);
        CleanupWinsock();
        return -1;
    }

    std::cout << "Server is listening on port " << PORT << "..." << std::endl;

    // Accept client connection
    clientSocket = accept(serverSocket, (struct sockaddr *)&clientAddr, &clientAddrSize);
    if (clientSocket == INVALID_SOCKET) {
        std::cerr << "Client connection failed." << std::endl;
        closesocket(serverSocket);
        CleanupWinsock();
        return -1;
    }

    std::cout << "Client connected." << std::endl;

    // Read data from result.txt and send it line by line
    std::ifstream file("result.txt");
    if (!file.is_open()) {
        std::cerr << "Failed to open result.txt" << std::endl;
        closesocket(clientSocket);
        closesocket(serverSocket);
        CleanupWinsock();
        return -1;
    }

    while (file.getline(buffer, MAX_BUFFER_SIZE)) {
        send(clientSocket, buffer, strlen(buffer), 0);
        send(clientSocket, "\n", 1, 0);  // Send newline for separation
    }

    std::cout << "Data sent to client." << std::endl;

    // Close sockets
    file.close();
    closesocket(clientSocket);
    closesocket(serverSocket);
    CleanupWinsock();

    return 0;
}

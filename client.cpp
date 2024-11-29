#include <iostream>
#include <winsock2.h>
#include <ws2tcpip.h>

#pragma comment(lib, "ws2_32.lib")

int main() {
    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        std::cerr << "WSAStartup failed." << std::endl;
        return 1;
    }

    SOCKET clientSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (clientSocket == INVALID_SOCKET) {
        std::cerr << "Socket creation failed." << std::endl;
        WSACleanup();
        return 1;
    }

    sockaddr_in serverAddr{};
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(8080); // Port 8080

    serverAddr.sin_addr.s_addr = inet_addr("127.0.0.1");
    if (serverAddr.sin_addr.s_addr == INADDR_NONE) {
        std::cerr << "Invalid address." << std::endl;
        closesocket(clientSocket);
        WSACleanup();
        return 1;
}

    if (connect(clientSocket, (sockaddr*)&serverAddr, sizeof(serverAddr)) == SOCKET_ERROR) {
        std::cerr << "Connection failed." << std::endl;
        closesocket(clientSocket);
        WSACleanup();
        return 1;
    }

    std::cout << "Connected to the server. Sending message..." << std::endl;

    // Send a start message to the server
    const char* startMessage = "START";
    send(clientSocket, startMessage, strlen(startMessage), 0);

    // Receive JSON data from server
    char buffer[1024];
    int bytesReceived;
    while ((bytesReceived = recv(clientSocket, buffer, sizeof(buffer) - 1, 0)) > 0) {
        buffer[bytesReceived] = '\0'; // Null-terminate received data
        std::cout << buffer;         // Print received JSON
    }

    if (bytesReceived == 0) {
        std::cout << "\nConnection closed by the server." << std::endl;
    } else if (bytesReceived == SOCKET_ERROR) {
        std::cerr << "Receive failed." << std::endl;
    }

    closesocket(clientSocket);
    WSACleanup();
    return 0;
}

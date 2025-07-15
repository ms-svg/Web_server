// src/socket/TCPWebServer.cpp
#include "../../include/socket/TCPWebServer.hpp"
#include <iostream>
#include <unistd.h>
#include <cstring>
#include <netinet/in.h>
#include <sys/socket.h>

TCPWebServer::TCPWebServer() : server_fd(-1) {}

bool TCPWebServer::start(int port) {
    struct sockaddr_in address{};
    server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd == 0) {
        perror("Socket failed");
        return false;
    }

    int opt = 1;
    // we are telling OS that let us reuse address and port immediately after this program close.
    setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &opt, sizeof(opt));

    address.sin_family = AF_INET;    // ipv4
    address.sin_addr.s_addr = INADDR_ANY;   // socket will listen for connections on all IP addresses assigned to the machine
    address.sin_port = htons(port);   

    if (bind(server_fd, (struct sockaddr*)&address, sizeof(address)) < 0) {
        perror("Bind failed");
        return false;
    }

    if (listen(server_fd, 5) < 0) {
        perror("Listen failed");
        return false;
    }

    std::cout << "🌐 Web Server started on port " << port << "\n";
    return true;
}

void TCPWebServer::handleRequests() {
    struct sockaddr_in client_addr{};
    socklen_t addrlen = sizeof(client_addr);

    while (true) {
        int client_fd = accept(server_fd, (struct sockaddr*)&client_addr, &addrlen);
        if (client_fd < 0) {
            perror("Accept failed");
            continue;
        }

        char buffer[3000] = {0};
        read(client_fd, buffer, 3000);
        std::cout << "📥 Request received:\n" << buffer << "\n";

        std::string http_response =
            "HTTP/1.1 200 OK\r\n"
            "Content-Type: text/html\r\n"
            "Content-Length: 46\r\n"
            "\r\n"
            "<html><body><h1>Hello World</h1></body></html>";

        send(client_fd, http_response.c_str(), http_response.length(), 0);
        close(client_fd);
    }
}

void TCPWebServer::stop() {
    close(server_fd);
}

TCPWebServer::~TCPWebServer() {
    stop();
}

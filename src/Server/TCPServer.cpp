#include "Server/TCPServer.hpp"
#include "Server/RequestHandler.hpp"
#include <iostream>
#include <cstring>
#include <arpa/inet.h>
#include <unistd.h>
#include <filesystem>
#include "Search/FileIndexer.hpp"
#include <sys/socket.h>
#include <filesystem>
namespace fs = std::filesystem;

TCPServer::TCPServer(const std::filesystem::path& base_path,FileIndexer& indexer_)
    : base_path(base_path), indexer(indexer_) {
    server_fd = -1;
}

void TCPServer::start(int port_) {
    server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd == -1) {
        perror("socket failed");
        exit(EXIT_FAILURE);
    }

    sockaddr_in address;
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(port_);

    if (bind(server_fd, (struct sockaddr*)&address, sizeof(address)) < 0) {
        perror("bind failed");
        close(server_fd);
        exit(EXIT_FAILURE);
    }

    if (listen(server_fd, 10) < 0) {
        perror("listen failed");
        close(server_fd);
        exit(EXIT_FAILURE);
    }

    std::cout << "Server listening on port " << port_ << "..." << std::endl;

    while (true) {
        int client_socket = accept(server_fd, nullptr, nullptr);
        if (client_socket < 0) {
            perror("accept failed");
            continue;
        }

        char buffer[4096] = {0};
        ssize_t bytes_read = read(client_socket, buffer, sizeof(buffer));

        if (bytes_read > 0) {
            std::string request(buffer, bytes_read);
            std::cout << "📥 Request received:\n" << request << "\n";
            RequestHandler requestHandler(base_path); // Create RequestHandler with base path
            requestHandler.handleClientRequest(client_socket, request, indexer);
        }

        close(client_socket);
    }

    close(server_fd);
}
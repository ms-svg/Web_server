#pragma once

#include <string>
#include <filesystem>
#include "Search/FileIndexer.hpp"

class TCPServer {
public:
    FileIndexer& indexer;
    std::filesystem::path base_path;
    TCPServer(const std::filesystem::path& base_path,
                FileIndexer& indexer);
    TCPServer(const FileIndexer& indexer);
    void start(int port);
    void stop();
    ~TCPServer() = default;
    
private:
    int server_fd;
};
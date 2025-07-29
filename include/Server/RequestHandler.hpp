#pragma once

#include <string>
#include <memory>
#include <filesystem>
#include "Search/FileIndexer.hpp"

class RequestHandler {
public:
    RequestHandler(const std::filesystem::path& basePath);
    void getRequestHandler(int client_fd, FileIndexer& indexer, std::string& path);
    void handleClientRequest(int client_fd, const std::string& rawRequest,
                             FileIndexer& indexer);

private:
    std::string urlMethod(const std::string& request);
    std::string googleFallback(const std::string& query, int client_fd);
    std::string fetchWebPage(const std::string& url);
    std::filesystem::path base_path_;
};
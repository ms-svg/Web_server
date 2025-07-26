#pragma once
#include <string>
#include <filesystem>

class TCPWebServer {
public:
    TCPWebServer(const std::filesystem::path& base_path);
    bool start(int port);
    void handleRequests();
    void stop();
    std::string getHostFromRequest(const char *);
    std::string getURLFromRequest(const char *);
    std::string fetch_webpage(const std::string& url);
    std::string urlDecode(const std::string& str);
    std::string googleFallback(const std::string& path, int client_fd);
    ~TCPWebServer();
private:
    int server_fd;
    std::filesystem::path base_path;
};

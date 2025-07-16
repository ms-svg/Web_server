#pragma once
#include <string>

class TCPWebServer {
public:
    TCPWebServer();
    bool start(int port);
    void handleRequests();
    void stop();
    std::string getHostFromRequest(const char *);
    std::string getURLFromRequest(const char *);
    std::string fetch_webpage(const std::string& url);
    ~TCPWebServer();
private:
    int server_fd;
};

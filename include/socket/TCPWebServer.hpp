#pragma once
#include <string>

class TCPWebServer {
public:
    TCPWebServer();
    bool start(int port);
    void handleRequests();
    void stop();
    ~TCPWebServer();
private:
    int server_fd;
};

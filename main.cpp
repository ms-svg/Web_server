#include "include/socket/TCPWebServer.hpp"

int main() {
    TCPWebServer server;

    if (!server.start(8080)) {
        return 1;
    }

    server.handleRequests();

    return 0;
}
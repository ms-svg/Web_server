#include "include/socket/TCPWebServer.hpp"
#include "utils/ConfigLoader.hpp"

int main() {
    // Load base path from config
    std::filesystem::path base_path = ConfigLoader::loadBasePath();

    // Create and start the server with the base path
    TCPWebServer server(base_path);

    if (!server.start(8080)) {
        return 1;
    }

    server.handleRequests();

    return 0;
}

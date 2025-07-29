#include "include/Server/TCPServer.hpp"
#include <filesystem>
#include "utils/ConfigLoader.hpp"
#include "include/Search/FileIndexer.hpp"
#include <iostream>


int main() {
    std::filesystem::path base_path = ConfigLoader::loadBasePath();
    if (base_path.empty()) {
        std::cerr << "Base path is not set in the configuration.\n";
        return 1;
    }

    // Build the index first
    FileIndexer indexer;
    indexer.buildIndex(base_path.string() + "/disk");
    std::cout << base_path << std::endl;
    std::cout << "Index built successfully.\n";

    TCPServer server(base_path, indexer);
    server.start(8080);
    return 0;
}
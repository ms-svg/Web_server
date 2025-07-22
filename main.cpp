#include "include/socket/TCPWebServer.hpp"
#include "utils/ConfigLoader.hpp"
#include "include/indexer/FileIndexer.hpp"

int main() {
    std::filesystem::path base_path = ConfigLoader::loadBasePath();

    // 🔍 Build the index first
    FileIndexer indexer;
    indexer.buildIndex((base_path.string()+"/disk"));
    std::cout << base_path << std::endl;
    std::cout << "Index built successfully.\n";

    // TODO: You can use indexer.search("keyword") from some endpoint

    TCPWebServer server(base_path);
    if (!server.start(8080)) {
        return 1;
    }
    server.handleRequests();

    return 0;
}

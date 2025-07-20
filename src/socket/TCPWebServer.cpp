#include "../../include/socket/TCPWebServer.hpp"
#include <iostream>
#include <unistd.h>
#include <cstring>
#include <netinet/in.h>
#include <sys/socket.h>
#include <curl/curl.h>
#include <fstream>
#include <sstream>
#include <filesystem>
namespace fs = std::filesystem;

TCPWebServer::TCPWebServer(const std::filesystem::path& basePath) 
    : server_fd(-1), base_path(basePath) {
    if (!fs::exists(basePath)) {
        std::cerr << "Base path does not exist: " << basePath << "\n";
        throw std::runtime_error("Invalid base path");
    }
}

bool TCPWebServer::start(int port) {
    struct sockaddr_in address{};
    server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd == 0) {
        perror("Socket failed");
        return false;
    }

    int opt = 1;
    // we are telling OS that let us reuse address and port immediately after this program close.
    setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &opt, sizeof(opt));

    address.sin_family = AF_INET;    // ipv4
    address.sin_addr.s_addr = INADDR_ANY;   // socket will listen for connections on all IP addresses assigned to the machine
    address.sin_port = htons(port);   

    if (bind(server_fd, (struct sockaddr*)&address, sizeof(address)) < 0) {
        perror("Bind failed");
        return false;
    }

    if (listen(server_fd, 5) < 0) {
        perror("Listen failed");
        return false;
    }

    std::cout << "🌐 Web Server started on port " << port << "\n";
    return true;
}

std::string TCPWebServer::getHostFromRequest(const char* request) {
    std::string req(request);
    size_t host_pos = req.find("Host:");
    if (host_pos == std::string::npos) return "";

    size_t start = host_pos + 5; // length of "Host:"
    size_t colon_pos = req.find(':', start);
    size_t newline_pos = req.find("\r\n", start);
    size_t end;
    if (colon_pos != std::string::npos && colon_pos < newline_pos) {
        end = colon_pos;
    } else {
        end = newline_pos;
    }
    std::string host = req.substr(start, end - start);
    host.erase(0, host.find_first_not_of(" \t"));
    host.erase(host.find_last_not_of(" \t") + 1);
    return host;
}

// Callback for storing fetched data into buffer.
static size_t WriteCallback(void* contents, size_t size, size_t nmemb, void* userp) {
    size_t totalSize = size * nmemb;
    std::string* response = static_cast<std::string*>(userp);
    response->append(static_cast<char*>(contents), totalSize);
    return totalSize;
}

// Fetching content for the given url.
std::string TCPWebServer::fetch_webpage(const std::string& url) {
    CURL* curl;
    CURLcode res;
    std::string readBuffer;

    curl = curl_easy_init();
    if (curl) {
        curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
        curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &readBuffer);
        curl_easy_setopt(curl, CURLOPT_USERAGENT, "Mozilla/5.0");

        // send request acting as a proxy.
        res = curl_easy_perform(curl);

        if (res != CURLE_OK) {
            std::cerr << "curl_easy_perform() failed: " << curl_easy_strerror(res) << std::endl;
            readBuffer = "";
        }

        // Cleanup
        curl_easy_cleanup(curl);
    } else {
        std::cerr << "Failed to init curl\n";
    }

    return readBuffer;
}


std::string TCPWebServer::getURLFromRequest(const char* request) {
    std::string req(request);
    size_t start = req.find("GET ");
    if (start == std::string::npos) return "";

    start += 4; 
    size_t end = req.find(" HTTP/", start);
    if (end == std::string::npos) return "";

    return req.substr(start, end - start); 
}



void TCPWebServer::handleRequests() {
    struct sockaddr_in client_addr{};
    socklen_t addrlen = sizeof(client_addr);

    while (true) {
        int client_fd = accept(server_fd, (struct sockaddr*)&client_addr, &addrlen);
        if (client_fd < 0) {
            perror("Accept failed");
            continue;
        }

        char buffer[3000] = {0};
        read(client_fd, buffer, 3000);

        std::cout << "📥 Request received:\n" << buffer << "\n";

        std::string path = getURLFromRequest(buffer); // e.g., /cat
      //  if (!path.empty() && path[0] == '/') path = path.substr(1); // remove leading '/'
        
        std::string local_file_path = (base_path.string()+"/disk" + path + ".html");
        std::cout<<local_file_path<<std::endl;

        if (fs::exists(local_file_path)) {
            std::ifstream file(local_file_path);
            std::stringstream content;
            content << file.rdbuf();
            std::string body = content.str();

            std::string response =
                "HTTP/1.1 200 OK\r\n"
                "Content-Type: text/html\r\n"
                "Content-Length: " + std::to_string(body.size()) + "\r\n"
                "\r\n" + body;

            send(client_fd, response.c_str(), response.length(), 0);
        } else {
            // Fallback: Google search
            std::string search_url = "https://www.google.com/search?q=" + path;
            std::cout<<search_url<<std::endl;
            std::string webpage = fetch_webpage(search_url);
            
            std::string response =
                "HTTP/1.1 200 OK\r\n"
                "Content-Type: text/html\r\n"
                "Content-Length: " + std::to_string(webpage.size()) + "\r\n"
                "\r\n" + webpage;

            send(client_fd, response.c_str(), response.length(), 0);
        }

        close(client_fd);
    }
}


void TCPWebServer::stop() {
    close(server_fd);
}

TCPWebServer::~TCPWebServer() {
    stop();
}

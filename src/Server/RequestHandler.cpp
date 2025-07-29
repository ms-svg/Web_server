#include "../../include/Server/RequestHandler.hpp"
#include "../../include/Search/Sorting.hpp"
#include "../../include/Search/FileIndexer.hpp"
#include <iostream>
#include <fstream>
#include <sstream>
#include <regex>
#include <curl/curl.h>
#include <unistd.h>
#include "Cache/LRUCache.hpp"

LRUCache* cache_ = new LRUCache(5); 

RequestHandler::RequestHandler( const std::filesystem::path& basePath)
    : base_path_(basePath) {}



std::string RequestHandler::urlMethod(const std::string& req) {
    size_t start = req.find("GET ");
    if (start != std::string::npos) return "GET";
    start = req.find("POST ");
    if (start != std::string::npos) return "POST";
    start = req.find("PUT ");
    if (start != std::string::npos) return "PUT";
    start = req.find("DELETE ");
    if (start != std::string::npos) return "DELETE";

    return "/"; // Default to root if no method found
}

static size_t WriteCallback(void* contents, size_t size, size_t nmemb, void* userp) {
    size_t totalSize = size * nmemb;
    std::string* response = static_cast<std::string*>(userp);
    response->append(static_cast<char*>(contents), totalSize);
    return totalSize;
}

std::string RequestHandler::fetchWebPage(const std::string& url) {
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

        res = curl_easy_perform(curl);

        if (res != CURLE_OK) {
            std::cerr << "curl_easy_perform() failed: " << curl_easy_strerror(res) << std::endl;
            readBuffer = "";
        }

        curl_easy_cleanup(curl);
    } else {
        std::cerr << "Failed to init curl\n";
    }

    return readBuffer;
}

std::string RequestHandler::googleFallback(const std::string& query, int client_fd) {
    std::cout << "Redirecting to Google search for: " << query << "\n";
    if (query.empty() || query == "/") {
        std::cout << "Please provide valid query" << std::endl;
        return "";
    }

    std::string search_url = "https://www.google.com/search?q=" + query;
    std::string webpage = fetchWebPage(search_url);

    std::string response =
        "HTTP/1.1 200 OK\r\n"
        "Content-Type: text/html\r\n"
        "Content-Length: " + std::to_string(webpage.size()) + "\r\n"
        "\r\n" + webpage;

    send(client_fd, response.c_str(), response.length(), 0);
    return webpage;
}

void RequestHandler::getRequestHandler(int client_fd, FileIndexer& indexer, std::string& path) {
    std::cout << "Parsed path: " << path << std::endl;

    if (path.rfind("/search?q=", 0) == 0 && path.size() > 10) {
        std::string query = path.substr(10);
        std::cout << "Searching for: " << query << "\n";

        std::vector<std::string> queryTokens = indexer.tokenize(query);
        if (queryTokens.empty()) {
            std::string cached = cache_->get(query);
            if (!cached.empty()) {
                std::cout << "Cache hit for: " << query << std::endl;
                std::string response =
                    "HTTP/1.1 200 OK\r\n"
                    "Content-Type: text/html\r\n"
                    "Content-Length: " + std::to_string(cached.size()) + "\r\n"
                    "\r\n" + cached;
                send(client_fd, response.c_str(), response.size(), 0);
            } else {
                std::string fetchedHtml = googleFallback(query, client_fd);
                cache_->put(query, fetchedHtml);
            }
            close(client_fd);
            return;
        }
        Sorting sorter(
            indexer.getInvertedIndex(),
            indexer.getDocumentLengths(),
            indexer.getTotalDocuments()
        );

        // Use either of these:
        std::vector<DocumentScore> rankedResults = sorter.rankByBM25(queryTokens);
        //std::vector<DocumentScore> rankedResults = sorter.rankByTFIDF(queryTokens);

        std::cout << "Results size: " << rankedResults.size() << std::endl;

        if (!rankedResults.empty()) {
            std::string html = "<html><body><h3>Search Results:</h3><ul>";
            for (const auto& result : rankedResults) {
                html += "<li><a href=\"/" + result.document + "\">" + result.document + "</a> (Score: " + std::to_string(result.score) + ")</li>";
            }
            html += "</ul></body></html>";
            std::string response =
                "HTTP/1.1 200 OK\r\n"
                "Content-Type: text/html\r\n"
                "Content-Length: " + std::to_string(html.size()) + "\r\n"
                "\r\n" + html;
            send(client_fd, response.c_str(), response.size(), 0);
        } else {
            // cache fallback
            std::string cached = cache_->get(query);
            if (!cached.empty()) {
                std::cout << "Cache hit for: " << query << std::endl;
                std::string response =
                    "HTTP/1.1 200 OK\r\n"
                    "Content-Type: text/html\r\n"
                    "Content-Length: " + std::to_string(cached.size()) + "\r\n"
                    "\r\n" + cached;
                send(client_fd, response.c_str(), response.size(), 0);
            } else {
                std::string fetchedHtml = googleFallback(query, client_fd);
                cache_->put(query, fetchedHtml);
            }
        }
    } else {
        std::string response =
            "HTTP/1.1 404 Not Found\r\n"
            "Content-Type: text/plain\r\n"
            "Content-Length: 9\r\n"
            "Connection: close\r\n"
            "\r\n"
            "Not Found";
        send(client_fd, response.c_str(), response.size(), 0);
    }

    close(client_fd);
    std::cout << "Response sent to client.\n";
}

void RequestHandler::handleClientRequest(int client_fd, const std::string& rawRequest,
                                         FileIndexer& indexer) {
    std::cout << "Request received:\n" << rawRequest << "\n";
    std::string method = urlMethod(rawRequest);
    std::cout << "Parsed URL Request: " << method << "\n";

    if( method == "GET" ) {
        size_t start = rawRequest.find(" ") + 1;
        size_t end = rawRequest.find(" HTTP/", start);
        if (end == std::string::npos) {
            std::cout << "Invalid request format.\n";
            std::string response =
                "HTTP/1.1 400 Bad Request\r\n"
                "Content-Type: text/plain\r\n"
                "Content-Length: 11\r\n"
                "\r\n"
                "Bad Request";
            send(client_fd, response.c_str(), response.size(), 0);
            close(client_fd);
            return;
        }
        std::string path = rawRequest.substr(start, end - start);
        std::cout << "Parsed path: " << path << std::endl;
        getRequestHandler(client_fd, indexer, path);
    }
    else if (method == "POST") {
        std::cout << "POST request received, but not implemented yet.\n";
        std::string response =
            "HTTP/1.1 501 Not Implemented\r\n"
            "Content-Type: text/plain\r\n"
            "Content-Length: 16\r\n"
            "\r\n"
            "Not Implemented";
        send(client_fd, response.c_str(), response.size(), 0);
        close(client_fd);
    } else {
        std::cout << "Unsupported request method: " << method << "\n";
        std::string response =
            "HTTP/1.1 400 Bad Request\r\n"
            "Content-Type: text/plain\r\n"
            "Content-Length: 11\r\n"
            "\r\n"
            "Bad Request";
        send(client_fd, response.c_str(), response.size(), 0);
        close(client_fd);
    }

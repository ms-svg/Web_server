#include "../../include/Server/RequestHandler.hpp"
#include "../../include/Search/Sorting.hpp"
#include "../../include/Search/FileIndexer.hpp"
#include <iostream>
#include <fstream>
#include <sstream>
#include <regex>
#include <curl/curl.h>
#include <unistd.h>
#include <ctime>
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

void RequestHandler::postRequestHandler(int clientSocket, const std::string& request) {
    std::cout << "Handling POST request.\n";

    // Find boundary from Content-Type
    std::string boundaryPrefix = "boundary=";
    size_t boundaryPos = request.find(boundaryPrefix);
    if (boundaryPos == std::string::npos) {
        std::cerr << "Boundary not found.\n";
        return;
    }

    // Extract boundary value (remove any trailing characters like \r\n)
    std::string boundaryValue = request.substr(boundaryPos + boundaryPrefix.length());
    size_t boundaryEnd = boundaryValue.find_first_of("\r\n");
    if (boundaryEnd != std::string::npos) {
        boundaryValue = boundaryValue.substr(0, boundaryEnd);
    }
    std::string boundary = "--" + boundaryValue;
    std::cout << "🔍 Boundary: [" << boundary << "]\n";
    
    size_t endOfHeaders = request.find("\r\n\r\n");
    if (endOfHeaders == std::string::npos) {
        std::cerr << "End of headers not found.\n";
        return;
    }

    std::string body = request.substr(endOfHeaders + 4);
    std::cout << "Body content:\n[" << body << "]\n";
    std::cout << "Body length: " << body.length() << "\n";
    
    // Check if this is a file upload with filename
    size_t fileStart = body.find("filename=");
    std::string filename;
    std::string fileContent;
    std::string folder;
    
    if (fileStart != std::string::npos) {
        // Handle file upload with filename
        std::cout << "Processing file upload with filename.\n";
        
        // Extract filename
        size_t startQuote = body.find("\"", fileStart);
        size_t endQuote = body.find("\"", startQuote + 1);
        filename = body.substr(startQuote + 1, endQuote - startQuote - 1);

        // Locate the beginning of the file data
        size_t contentStart = body.find("\r\n\r\n", endQuote);
        if (contentStart == std::string::npos) {
            std::cerr << "File content start not found.\n";
            return;
        }
        contentStart += 4;

        size_t contentEnd = body.find("\r\n" + boundary, contentStart);
        if (contentEnd == std::string::npos) {
            contentEnd = body.find(boundary, contentStart);
        }
        if (contentEnd != std::string::npos) {
            fileContent = body.substr(contentStart, contentEnd - contentStart);
            // Remove any trailing \r\n
            while (!fileContent.empty() && (fileContent.back() == '\r' || fileContent.back() == '\n')) {
                fileContent.pop_back();
            }
        } else {
            fileContent = body.substr(contentStart);
        }

        // Decide folder based on extension
        if (filename.find(".txt") != std::string::npos)
            folder = "disk/TEXT/";
        else if (filename.find(".html") != std::string::npos)
            folder = "disk/HTML/";
        else {
            std::cerr << "Unsupported file type: " << filename << "\n";
            return;
        }
    } else {
        // Handle simple POST data without filename
        std::cout << "Processing POST data without filename.\n";
        
        // Look for form field name (e.g., name="data" or name="content")
        size_t nameStart = body.find("name=\"");
        if (nameStart != std::string::npos) {
            size_t nameEndQuote = body.find("\"", nameStart + 6);
            std::string fieldName = body.substr(nameStart + 6, nameEndQuote - nameStart - 6);
            std::cout << "Found field: " << fieldName << "\n";
            
            // Get content after the headers
            size_t contentStart = body.find("\r\n\r\n", nameEndQuote);
            if (contentStart != std::string::npos) {
                contentStart += 4;
                size_t contentEnd = body.find("\r\n" + boundary, contentStart);
                if (contentEnd == std::string::npos) {
                    contentEnd = body.find(boundary, contentStart);
                }
                if (contentEnd != std::string::npos) {
                    fileContent = body.substr(contentStart, contentEnd - contentStart);
                    // Remove any trailing \r\n
                    while (!fileContent.empty() && (fileContent.back() == '\r' || fileContent.back() == '\n')) {
                        fileContent.pop_back();
                    }
                } else {
                    fileContent = body.substr(contentStart);
                }
                
                // Generate a default filename based on content type or field name
                filename = fieldName + "_" + std::to_string(time(nullptr)) + ".txt";
                folder = "disk/TEXT/";
            }
        } else {
            // Fallback: treat entire body as text content
            std::cout << "No form field found, treating as raw text.\n";
            fileContent = body;
            filename = "upload_" + std::to_string(time(nullptr)) + ".txt";
            folder = "disk/TEXT/";
        }
    }

    if (filename.empty() || fileContent.empty()) {
        std::cerr << "Failed to extract filename or content.\n";
        std::string response = "HTTP/1.1 400 Bad Request\r\nContent-Type: text/plain\r\n\r\nInvalid upload data\n";
        send(clientSocket, response.c_str(), response.size(), 0);
        return;
    }

    std::string fullPath = folder + filename;
    std::ofstream outFile(fullPath);
    if (!outFile) {
        std::cerr << "Failed to open file for writing: " << fullPath << "\n";
        std::string response = "HTTP/1.1 500 Internal Server Error\r\nContent-Type: text/plain\r\n\r\nFailed to save file\n";
        send(clientSocket, response.c_str(), response.size(), 0);
        return;
    }

    outFile << fileContent;
    outFile.close();

    std::cout << "File saved: " << fullPath << "\n";
    std::string response = "HTTP/1.1 200 OK\r\nContent-Type: text/plain\r\n\r\nFile uploaded to " + folder + " as " + filename + "\n";
    send(clientSocket, response.c_str(), response.size(), 0);
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
            std::cout << "Handling POST request.\n";
            postRequestHandler(client_fd, rawRequest);
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
}

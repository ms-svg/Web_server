#pragma once
#include "../include/nlohmann/json.hpp"
#include <string>
#include <fstream>
#include <filesystem>
#include <iostream>

namespace fs = std::filesystem;

class ConfigLoader {
public:
    static fs::path loadBasePath(const std::string& configFile = "config/config.json") {
        std::ifstream file(configFile);
        if (!file.is_open()) {
            std::cerr << "Failed to open config: " << configFile << "\n";
            return fs::current_path(); // fallback to current dir
        }

        nlohmann::json config;
        try {
            file >> config;
        } catch (const nlohmann::json::parse_error& e) {
            std::cerr << "JSON parse error: " << e.what() << '\n';
            return fs::current_path();
        }

        std::string raw_base_path = config.value("base_path", ".");
        return fs::absolute(raw_base_path);
    }
};

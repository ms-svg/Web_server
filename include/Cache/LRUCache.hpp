#pragma once

#include <unordered_map>
#include <list>
#include <string>

class LRUCache {
public:
    LRUCache(size_t capacity);

    bool contains(const std::string& key) const;
    std::string get(const std::string& key);
    void put(const std::string& key, const std::string& value);

private:
    size_t capacity_;
    std::list<std::string> keys_; 
    std::unordered_map<std::string, std::pair<std::string, std::list<std::string>::iterator>> cache_;
};

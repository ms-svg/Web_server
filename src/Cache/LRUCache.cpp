#include "../../include/Cache/LRUCache.hpp"

LRUCache::LRUCache(size_t capacity) : capacity_(capacity) {}

bool LRUCache::contains(const std::string& key) const {
    return cache_.find(key) != cache_.end();
}

std::string LRUCache::get(const std::string& key) {
    auto it = cache_.find(key);
    if (it == cache_.end()) return "";

    keys_.erase(it->second.second);
    keys_.push_front(key);
    it->second.second = keys_.begin();

    return it->second.first;
}

void LRUCache::put(const std::string& key, const std::string& value) {
    auto it = cache_.find(key);
    if (it != cache_.end()) {
        keys_.erase(it->second.second);
        cache_.erase(it);
    }

    keys_.push_front(key);
    cache_[key] = { value, keys_.begin() };

    if (cache_.size() > capacity_) {
        std::string oldKey = keys_.back();
        keys_.pop_back();
        cache_.erase(oldKey);
    }
}

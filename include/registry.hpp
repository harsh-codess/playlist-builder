#pragma once

#include <algorithm>
#include <functional>
#include <map>
#include <string>
#include <vector>

#include "exceptions.hpp"

// Generic id-keyed store that remembers insertion order. T must expose
// `const std::string& id() const`.
template <typename T>
class Registry {
public:
    explicit Registry(std::string prefix, std::string noun)
        : prefix_(std::move(prefix)), noun_(std::move(noun)) {}

    std::string nextId() const {
        long long n = next_;
        while (items_.count(prefix_ + std::to_string(n))) ++n;
        return prefix_ + std::to_string(n);
    }

    long long peekNextId() const { return next_; }
    void setNextId(long long next) { next_ = std::max<long long>(1, next); }

    T& add(T item) {
        const std::string id = item.id();
        if (items_.count(id)) throw DuplicateException(noun_ + " " + id + " already exists");
        reserve(id);
        order_.push_back(id);
        return items_.emplace(id, std::move(item)).first->second;
    }

    T& get(const std::string& id) {
        auto it = items_.find(id);
        if (it == items_.end()) throw NotFoundException("No " + noun_ + " with id " + id);
        return it->second;
    }

    const T& get(const std::string& id) const {
        auto it = items_.find(id);
        if (it == items_.end()) throw NotFoundException("No " + noun_ + " with id " + id);
        return it->second;
    }

    void remove(const std::string& id) {
        if (!items_.erase(id)) throw NotFoundException("No " + noun_ + " with id " + id);
        order_.erase(std::remove(order_.begin(), order_.end(), id), order_.end());
    }

    void forEach(const std::function<void(T&)>& fn) {
        for (const auto& id : order_) fn(items_.at(id));
    }

    void forEach(const std::function<void(const T&)>& fn) const {
        for (const auto& id : order_) fn(items_.at(id));
    }

    void clear() {
        items_.clear();
        order_.clear();
        next_ = 1;
    }

    std::size_t size() const { return items_.size(); }

private:
    void reserve(const std::string& id) {
        if (id.rfind(prefix_, 0) != 0) return;
        try {
            std::size_t used = 0;
            long long n = std::stoll(id.substr(prefix_.size()), &used);
            if (used == id.size() - prefix_.size() && n >= next_) next_ = n + 1;
        } catch (const std::exception&) {
        }
    }

    std::string prefix_;
    std::string noun_;
    std::map<std::string, T> items_;
    std::vector<std::string> order_;
    long long next_ = 1;
};

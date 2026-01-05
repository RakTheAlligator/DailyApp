#pragma once
#include <string>
#include <vector>
#include "WeightEntry.hpp"

class Storage {
public:
    explicit Storage(std::string csvPath);

    // Load all entries from CSV (sorted by date ascending)
    std::vector<WeightEntry> loadAll() const;

    // Append a new entry (adds header if file is new)
    void append(const WeightEntry& e) const;

private:
    std::string path_;

    void ensureHeaderIfNeeded() const;
};

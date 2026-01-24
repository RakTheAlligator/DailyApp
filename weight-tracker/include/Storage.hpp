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

    // Replace entry if date exists; otherwise insert; keeps CSV sorted by date
    // Returns true if replaced, false if inserted
    bool upsertByDate(const WeightEntry& e) const;

    // Remove entry for a given date; returns true if removed
    bool removeByDate(const std::string& date) const;

private:
    std::string path_;

    void ensureHeaderIfNeeded() const;
    void rewriteAll(const std::vector<WeightEntry>& rows) const;
};

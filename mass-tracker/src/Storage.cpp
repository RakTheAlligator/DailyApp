#include "Storage.hpp"

#include <algorithm>
#include <fstream>
#include <sstream>

static std::string trim(const std::string& s) {
    const auto a = s.find_first_not_of(" \t\r\n");
    const auto b = s.find_last_not_of(" \t\r\n");
    if (a == std::string::npos) return "";
    return s.substr(a, b - a + 1);
}

Storage::Storage(std::string csvPath) : path_(std::move(csvPath)) {}

void Storage::ensureHeaderIfNeeded() const {
    std::ifstream in(path_);
    if (in.good() && in.peek() != std::ifstream::traits_type::eof()) return;

    std::ofstream out(path_, std::ios::trunc);
    out << "date,weight_kg\n";
}

std::vector<WeightEntry> Storage::loadAll() const {
    ensureHeaderIfNeeded();

    std::ifstream in(path_);
    std::vector<WeightEntry> rows;

    std::string line;
    bool first = true;
    while (std::getline(in, line)) {
        if (first) { first = false; continue; }  // skip header
        line = trim(line);
        if (line.empty()) continue;

        // very simple CSV: date,weight
        std::stringstream ss(line);
        std::string date, weightStr;

        if (!std::getline(ss, date, ',')) continue;
        if (!std::getline(ss, weightStr)) continue;

        WeightEntry e;
        e.date = trim(date);

        try {
            e.weightKg = std::stod(trim(weightStr));
        } catch (...) {
            continue;
        }

        rows.push_back(e);
    }

    std::sort(rows.begin(), rows.end(),
              [](const WeightEntry& a, const WeightEntry& b) { return a.date < b.date; });

    return rows;
}

void Storage::append(const WeightEntry& e) const {
    ensureHeaderIfNeeded();

    std::ofstream out(path_, std::ios::app);
    out << e.date << "," << e.weightKg << "\n";
}

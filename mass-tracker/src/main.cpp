#include <iostream>
#include <string>
#include <vector>
#include <filesystem>

#include "Storage.hpp"

static std::string trim(const std::string& s) {
    const auto a = s.find_first_not_of(" \t\r\n");
    const auto b = s.find_last_not_of(" \t\r\n");
    if (a == std::string::npos) return "";
    return s.substr(a, b - a + 1);
}

static std::string askDate() {
    while (true) {
        std::cout << "Date (YYYY-MM-DD): ";
        std::string d;
        std::getline(std::cin, d);
        d = trim(d);
        if (d.size() == 10 && d[4] == '-' && d[7] == '-') return d;
        std::cout << "Format invalide. Exemple: 2026-01-05\n";
    }
}

static double askWeightKg() {
    while (true) {
        std::cout << "Poids (kg): ";
        std::string s;
        std::getline(std::cin, s);
        s = trim(s);
        try {
            double w = std::stod(s);
            if (w > 0.0 && w < 1000.0) return w;
        } catch (...) {}
        std::cout << "Valeur invalide. Exemple: 74.6\n";
    }
}

static void printHistory(const std::vector<WeightEntry>& rows) {
    if (rows.empty()) {
        std::cout << "\nHistorique vide.\n";
        return;
    }
    std::cout << "\nHistorique du poids:\n";
    for (const auto& e : rows) {
        std::cout << "  " << e.date << "  ->  " << e.weightKg << " kg\n";
    }
    if (rows.size() >= 2) {
        const auto& prev = rows[rows.size() - 2];
        const auto& last = rows.back();
        const double delta = last.weightKg - prev.weightKg;
        std::cout << "\nDernier changement: " << delta << " kg ("
                  << prev.date << " -> " << last.date << ")\n";
    }
}

static std::filesystem::path computeCsvPath(const char* argv0) {
    // argv0 -> .../build/bin/mass_tracker
    std::filesystem::path exePath = std::filesystem::weakly_canonical(argv0);
    auto binDir = exePath.parent_path();         // .../build/bin
    auto buildDir = binDir.parent_path();        // .../build

    // We want: .../DailyApp/mass-tracker/data/weights.csv
    // buildDir is .../DailyApp/build
    auto repoRoot = buildDir.parent_path();      // .../DailyApp

    return repoRoot / "mass-tracker" / "data" / "weights.csv";
}

int main(int argc, char** argv) {
    std::cout << "Mass Tracker v0.1\n";
    std::cout << "-----------------\n";

    const auto csvPath = computeCsvPath(argv[0]);
    std::filesystem::create_directories(csvPath.parent_path());

    Storage storage(csvPath.string());

    const std::string date = askDate();
    const double weight = askWeightKg();

    storage.append(WeightEntry{date, weight});

    const auto rows = storage.loadAll();
    printHistory(rows);

    return 0;
}

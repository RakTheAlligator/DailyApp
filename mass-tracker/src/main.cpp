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
static double kgToLb(double kg) { return kg * 2.20462262185; }
static double lbToKg(double lb) { return lb / 2.20462262185; }

static bool parseWeightWithUnit(const std::string& input, double& outKg) {
    // Expected: "<number> <unit>" where unit is "kg" or "lb"
    // Examples: "65.0 kg" or "143.3 lb"
    std::string s = trim(input);

    // Split on whitespace
    std::stringstream ss(s);
    std::string valueStr, unit;
    if (!(ss >> valueStr >> unit)) {
        return false; // missing one part
    }

    // Reject extra tokens (optional strictness)
    std::string extra;
    if (ss >> extra) return false;

    double value = 0.0;
    try {
        value = std::stod(valueStr);
    } catch (...) {
        return false;
    }

    if (value <= 0.0 || value >= 1000.0) return false;

    // Normalize unit to lowercase
    for (auto& c : unit) c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));

    if (unit == "kg") {
        outKg = value;
        return true;
    }
    if (unit == "lb" || unit == "lbs") {
        outKg = lbToKg(value);
        return true;
    }

    return false;
}

static double askWeightKg() {
    while (true) {
        std::cout << "Poids (kg ou lb) : ";
        std::string s;
        std::getline(std::cin, s);

        double kg = 0.0;
        if (parseWeightWithUnit(s, kg)) return kg;

        std::cout << "Entree invalide. Exemple: 65.0 kg  ou  143.3 lb\n";
        std::cout << "⚠️ L'unite est obligatoire (kg/lb).\n";
    }
}


static void printHistory(const std::vector<WeightEntry>& rows) {
    if (rows.empty()) {
        std::cout << "\nHistorique vide.\n";
        return;
    }

    std::cout << "\nHistorique du poids:\n";
    for (const auto& e : rows) {
        const double lb = kgToLb(e.weightKg);
        std::cout << "  " << e.date << "  ->  " << e.weightKg << " kg"
                  << " (" << lb << " lb)\n";
    }

    if (rows.size() >= 2) {
        const auto& prev = rows[rows.size() - 2];
        const auto& last = rows.back();
        const double deltaKg = last.weightKg - prev.weightKg;
        const double deltaLb = kgToLb(last.weightKg) - kgToLb(prev.weightKg);

        std::cout << "\nDernier changement: " << deltaKg << " kg"
                  << " (" << deltaLb << " lb) "
                  << "(" << prev.date << " -> " << last.date << ")\n";
    }
}


static std::filesystem::path computeCsvPath() {
    return std::filesystem::path(MASS_TRACKER_DATA_DIR) / "weights.csv";
}

int main([[maybe_unused]] int argc, char** argv) { 
    
    std::cout << "Mass Tracker v0.1\n";
    std::cout << "-----------------\n";

    const auto csvPath = computeCsvPath();                   
    std::filesystem::create_directories(csvPath.parent_path()); 

    Storage storage(csvPath.string());

    const std::string date = askDate();
    const double weight = askWeightKg();

    storage.append(WeightEntry{date, weight});

    const auto rows = storage.loadAll();
    printHistory(rows);

    return 0;
}

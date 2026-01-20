#include <iostream>
#include <string>
#include <vector>
#include <filesystem>
#include <fstream>
#include <cstdlib>
#include <sstream>
#include <cctype>
#include <unistd.h>

#include "Storage.hpp"

static bool stdinIsInteractive() {
    return ::isatty(STDIN_FILENO);
}

static std::string trim(const std::string& s) {
    const auto a = s.find_first_not_of(" \t\r\n");
    const auto b = s.find_last_not_of(" \t\r\n");
    if (a == std::string::npos) return "";
    return s.substr(a, b - a + 1);
}

static std::string askDateOrEmpty() {
    while (true) {
        std::cout << "Date (YYYY-MM-DD) [Entrée = voir historique] : ";
        std::string d;
        std::getline(std::cin, d);
        d = trim(d);

        if (d.empty()) return ""; // mode consultation

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

static std::string shellQuote(const std::string& s) {
    std::string out = "'";
    for (char c : s) {
        if (c == '\'') out += "'\\''";
        else out += c;
    }
    out += "'";
    return out;
}

static void plotHistoryPng(const std::vector<WeightEntry>& rows) {
    if (rows.size() < 2) return;

    const auto dataDir = std::filesystem::path(MASS_TRACKER_DATA_DIR);
    std::filesystem::create_directories(dataDir);

    const auto csvPath = (dataDir / "weights.csv").string();
    const auto outPng  = (dataDir / "weight_history.png").string();

    // Repo root: MASS_TRACKER_DATA_DIR = .../mass-tracker/data
    // parent_path() -> .../mass-tracker
    // parent_path() -> .../DailyApp
    const auto repoRoot = dataDir.parent_path().parent_path();

    const auto scriptPath = (repoRoot / "mass-tracker" / "analytics" / "plot_weights.py");

    // Utiliser python du venv si dispo, sinon python3
    const auto venvPython = (repoRoot / ".venv" / "bin" / "python");
    const std::string pythonExe = std::filesystem::exists(venvPython)
        ? venvPython.string()
        : "python3";

    std::string cmd;
    cmd += shellQuote(pythonExe);
    cmd += " ";
    cmd += shellQuote(scriptPath.string());
    cmd += " --csv ";
    cmd += shellQuote(csvPath);
    cmd += " --out ";
    cmd += shellQuote(outPng);
    cmd += " >/dev/null 2>&1";

    const int ret = std::system(cmd.c_str());
    if (ret != 0) {
        std::cerr << "Erreur: plot python a echoue (code " << ret << ")\n";
        std::cerr << "Commande: " << cmd << "\n";
        return;
    }

    std::cout << "Graph genere: " << std::filesystem::absolute(outPng) << "\n";
}


int main() { 

    std::cout << "Mass Tracker v0.1\n";
    std::cout << "-----------------\n";

    const auto csvPath = computeCsvPath();                   
    std::filesystem::create_directories(csvPath.parent_path()); 

    Storage storage(csvPath.string());

    if (!stdinIsInteractive()) {
        const auto rows = storage.loadAll();
        printHistory(rows);
        plotHistoryPng(rows);
        return 0;
    }
    
    const std::string date = askDateOrEmpty();
    
    if (date.empty()) {
        // Mode consultation: pas d'ajout
        const auto rows = storage.loadAll();
        printHistory(rows);
        plotHistoryPng(rows); // génère le PNG et affiche le chemin
        return 0;
    }

    // Mode ajout
    const double weight = askWeightKg();
    storage.append(WeightEntry{date, weight});

    const auto rows = storage.loadAll();
    printHistory(rows);
    plotHistoryPng(rows);


    return 0;
}

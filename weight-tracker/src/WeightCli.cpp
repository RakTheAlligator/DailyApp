#include "WeightCli.hpp"
#include <iostream>
#include <vector>
#include <string>
#include <span>

#include <cctype>
#include <filesystem>
#include <cstdlib>
#include <sstream>
#include <span>
#include <string_view>


#include "Storage.hpp"



namespace {

void print_weight_help() {
    std::cout <<
R"(Usage:
  ./DailyApp weight add <YYYY-MM-DD> <weight><kg|lb>
  ./DailyApp weight remove <YYYY-MM-DD>
  ./DailyApp weight history
)";
}

} // namespace

namespace weight {

static bool isValidDateYYYYMMDD(const std::string& d) {
    if (d.size() != 10) return false;
    if (d[4] != '-' || d[7] != '-') return false;
    for (size_t i : {0u,1u,2u,3u,5u,6u,8u,9u}) {
        if (!std::isdigit(static_cast<unsigned char>(d[i]))) return false;
    }
    return true;
}

static double lbToKg(double lb) { return lb / 2.20462262185; }
static double kgToLb(double kg) { return kg * 2.20462262185; }

static bool parseWeightTokenToKg(const std::string& token, double& outKg) {
    std::string s = token;
    while (!s.empty() && std::isspace((unsigned char)s.front())) s.erase(s.begin());
    while (!s.empty() && std::isspace((unsigned char)s.back())) s.pop_back();
    if (s.empty()) return false;

    size_t pos = s.size();
    while (pos > 0 && std::isalpha(static_cast<unsigned char>(s[pos - 1]))) pos--;
    if (pos == s.size()) return false; // no unit

    std::string valueStr = s.substr(0, pos);
    std::string unit = s.substr(pos);
    for (auto& c : unit) c = static_cast<char>(std::tolower((unsigned char)c));

    double value = 0.0;
    try { value = std::stod(valueStr); } catch (...) { return false; }
    if (value <= 0.0 || value >= 1000.0) return false;

    if (unit == "kg") { outKg = value; return true; }
    if (unit == "lb" || unit == "lbs") { outKg = lbToKg(value); return true; }
    return false;
}

static void printHistory(const std::vector<WeightEntry>& rows) {
    if (rows.empty()) {
        std::cout << "Historique vide.\n";
        return;
    }

    std::cout << "Historique du poids:\n";
    for (const auto& e : rows) {
        std::cout << "  " << e.date
                  << "  ->  " << e.weightKg << " kg"
                  << " (" << kgToLb(e.weightKg) << " lb)\n";
    }

    if (rows.size() >= 2) {
        const auto& prev = rows[rows.size() - 2];
        const auto& last = rows.back();
        std::cout << "\nDernier changement: "
                  << (last.weightKg - prev.weightKg) << " kg ("
                  << (kgToLb(last.weightKg) - kgToLb(prev.weightKg)) << " lb) "
                  << "(" << prev.date << " -> " << last.date << ")\n";
    }
}

static std::filesystem::path computeCsvPath() {
    return std::filesystem::path(DAILYAPP_DATA_DIR) / "weight_history.csv";
}

static int runWeightHistoryPlot() {
    const std::filesystem::path root = std::filesystem::path(DAILYAPP_ROOT_DIR);

    const std::filesystem::path csv = computeCsvPath();
    const std::filesystem::path out = std::filesystem::path(DAILYAPP_DATA_DIR) / "weight_history.png";

    // analytics est uniquement dans /DailyApp/analytics
    const std::filesystem::path py = root / "analytics" / "weight_history.py";

    if (!std::filesystem::exists(py)) {
        std::cerr << "[plot] Script not found: " << py << "\n";
        return 127;
    }

    std::ostringstream cmd;
    cmd << "python3 "
        << "\"" << py.string() << "\""
        << " --csv " << "\"" << csv.string() << "\""
        << " --out " << "\"" << out.string() << "\"";

    const int rc = std::system(cmd.str().c_str());
    if (rc != 0) {
        std::cerr << "[plot] analytics failed (exit code " << rc << ")\n";
    }
    return rc;
}

int run(std::span<const std::string_view> args) {
    if (args.empty() || args[0] == "--help" || args[0] == "-h") {
        print_weight_help();
        return 0;
    }

    const std::string_view cmd = args[0];
    Storage storage(computeCsvPath().string());

    if (cmd == "history") {
        if (args.size() != 1) { print_weight_help(); return 1; }
        printHistory(storage.loadAll());
        (void)runWeightHistoryPlot();
        return 0;
    }

    if (cmd == "add") {
        if (args.size() != 3) { print_weight_help(); return 1; }
        const std::string date(args[1]);
        const std::string wtok(args[2]);

        if (!isValidDateYYYYMMDD(date)) {
            std::cerr << "Date invalide. Exemple: 2026-01-24\n";
            return 2;
        }

        double kg = 0.0;
        if (!parseWeightTokenToKg(wtok, kg)) {
            std::cerr << "Poids invalide. Exemple: 62kg ou 143lb\n";
            return 2;
        }

        WeightEntry e{date, kg};
        const bool replaced = storage.upsertByDate(e);
        std::cout << (replaced ? "Mis a jour: " : "Ajoute: ")
                  << date << " -> " << kg << " kg\n";
        return 0;
    }

    if (cmd == "remove") {
        if (args.size() != 2) { print_weight_help(); return 1; }
        const std::string date(args[1]);

        if (!isValidDateYYYYMMDD(date)) {
            std::cerr << "Date invalide. Exemple: 2026-01-24\n";
            return 2;
        }

        const bool removed = storage.removeByDate(date);
        if (!removed) {
            std::cerr << "Aucune entree a supprimer pour la date " << date << "\n";
            return 3;
        }

        std::cout << "Supprime: " << date << "\n";
        return 0;
    }

    std::cerr << "Unknown weight command: " << cmd << "\n";
    print_weight_help();
    return 2;
}

} // namespace weight

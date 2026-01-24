#include <cctype>
#include <filesystem>
#include <iostream>
#include <string>
#include <vector>
#include <cstdlib>  // en haut du fichier
#include <sstream>

#include "Storage.hpp"

static bool isValidDateYYYYMMDD(const std::string& d) {
    if (d.size() != 10) return false;
    if (d[4] != '-' || d[7] != '-') return false;
    for (size_t i : {0u,1u,2u,3u,5u,6u,8u,9u}) {
        if (!std::isdigit(static_cast<unsigned char>(d[i]))) return false;
    }
    // Validation calendrier complète pas nécessaire ici (tu peux l’ajouter plus tard)
    return true;
}

static double lbToKg(double lb) { return lb / 2.20462262185; }
static double kgToLb(double kg) { return kg * 2.20462262185; }

static bool parseWeightTokenToKg(const std::string& token, double& outKg) {
    // Accept: 62kg, 62.5kg, 143lb, 143lbs
    // Also accept: "62" (no unit) -> reject (unit mandatory)
    std::string s = token;
    // trim minimal
    while (!s.empty() && std::isspace((unsigned char)s.front())) s.erase(s.begin());
    while (!s.empty() && std::isspace((unsigned char)s.back())) s.pop_back();
    if (s.empty()) return false;

    // Split value+unit by scanning from end where letters start
    size_t pos = s.size();
    while (pos > 0 && std::isalpha(static_cast<unsigned char>(s[pos - 1]))) pos--;

    if (pos == s.size()) return false; // no unit
    std::string valueStr = s.substr(0, pos);
    std::string unit = s.substr(pos);

    // normalize unit
    for (auto& c : unit) c = static_cast<char>(std::tolower((unsigned char)c));

    double value = 0.0;
    try {
        value = std::stod(valueStr);
    } catch (...) {
        return false;
    }
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

static int runWeightHistoryPlot() {
    const std::filesystem::path root = std::filesystem::path(DAILYAPP_ROOT_DIR);
    const std::filesystem::path csv  = std::filesystem::path(WEIGHT_TRACKER_DATA_DIR) / "weight_history.csv";
    const std::filesystem::path out  = std::filesystem::path(WEIGHT_TRACKER_DATA_DIR) / "weight_history.png";
    const std::filesystem::path py   = root / "analytics" / "weight_history.py";

    if (!std::filesystem::exists(py)) {
        std::cerr << "[plot] Script not found: " << py << "\n";
        return 127;
    }

    // Quotes for paths with spaces
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

static std::filesystem::path computeCsvPath() {
    // WEIGHT_TRACKER_DATA_DIR = <DailyApp>/data
    return std::filesystem::path(WEIGHT_TRACKER_DATA_DIR) / "weight_history.csv";
}

static void usage() {
    std::cout <<
R"(Usage:
  ./weight_tracker weight add <YYYY-MM-DD> <weight><kg|lb>
  ./weight_tracker weight remove <YYYY-MM-DD>
  ./weight_tracker weight history
)";
}

int main(int argc, char** argv) {
    if (argc < 2) { usage(); return 1; }

    const std::string top = argv[1];
    if (top == "-h" || top == "--help" || top == "help") { usage(); return 0; }

    if (top != "weight") {
        std::cerr << "Commande inconnue: " << top << "\n";
        usage();
        return 2;
    }

    if (argc < 3) { usage(); return 1; }
    const std::string cmd = argv[2];

    Storage storage(computeCsvPath().string());

    if (cmd == "history") {
        printHistory(storage.loadAll());
        runWeightHistoryPlot(); // side-effect: generate PNG
        return 0;
    }


    if (cmd == "add") {
        if (argc != 5) { usage(); return 1; }

        const std::string date = argv[3];
        const std::string wtok = argv[4];

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
        if (argc != 4) { usage(); return 1; }

        const std::string date = argv[3];
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

    std::cerr << "Sous-commande inconnue: " << cmd << "\n";
    usage();
    return 2;
}

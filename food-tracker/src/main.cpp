#include "ProductDB.hpp"
#include "Calculator.hpp"
#include "Csv.hpp"
#include "Date.hpp"
#include <iostream>
#include <iomanip>
#include <filesystem>
#include <algorithm>


static void ensure_headers(const std::string& products,
                           const std::string& batches,
                           const std::string& extras) {
    if (!file_exists(products)) {
    append_line(products, "id,name,unit,kcal_per_100,aliases");
    }
    if (!file_exists(batches)) {
    append_line(batches, "batch_id,start_date,days,product_id,qty,unit,comment");
    }
    if (!file_exists(extras)) {
    append_line(extras, "date,kcal,comment");
    }
}

static bool parse_qty_unit(const std::string& s, double& qty, std::string& unit) {
    // ex: "700g" ou "250ml"
    std::string t = trim(s);
    if (t.size() < 2) return false;
    size_t pos = t.find_first_not_of("0123456789.");
    if (pos == std::string::npos) return false;
    qty = std::stod(t.substr(0, pos));
    unit = t.substr(pos);
    unit = trim(unit);
    if (unit == "ml") unit = "mL";
    return (unit == "g" || unit == "mL");
}
static std::filesystem::path dataDir() {
    return std::filesystem::path(FOOD_TRACKER_DATA_DIR);
}
static std::string join_rest_args(int argc, char** argv, int start_idx) {
    std::string out;
    for (int i = start_idx; i < argc; ++i) {
        if (i > start_idx) out += " ";
        out += argv[i];
    }
    return out;
}

static std::filesystem::path draftPath() {
    return std::filesystem::path(FOOD_TRACKER_DATA_DIR) / "draft.csv";
}

static void draft_clear() {
    const auto p = draftPath();
    if (std::filesystem::exists(p)) std::filesystem::remove(p);
}

static bool draft_exists() {
    return std::filesystem::exists(draftPath());
}

struct DraftMeta { Date start{}; int days=0; };

static bool draft_read_meta(DraftMeta& meta) {
    auto lines = read_lines(draftPath().string());
    if (lines.size() < 2) return false;
    // line0: start_date,days
    // line1: 2026-01-17,7
    auto cols = split_csv_simple(lines[1]);
    if (cols.size() < 2) return false;
    if (!parse_date_yyyy_mm_dd(cols[0], meta.start)) return false;
    meta.days = std::stoi(cols[1]);
    return meta.days > 0;
}

static void draft_init(const Date& start, int days) {
    draft_clear();
    const auto p = draftPath().string();
    append_line(p, "start_date,days");
    append_line(p, format_date(start) + "," + std::to_string(days));
    append_line(p, "product_id,qty,unit,comment");
}

static void draft_add_line(const std::string& product_id, double qty, const std::string& unit, const std::string& comment) {
    append_line(draftPath().string(),
        product_id + "," + std::to_string(qty) + "," + unit + "," + comment
    );
}

struct DraftItem { std::string pid; double qty=0.0; std::string unit; std::string comment; };

static std::vector<DraftItem> draft_read_items() {
    std::vector<DraftItem> items;
    auto lines = read_lines(draftPath().string());
    // header 0, meta 1, header items 2, data from 3
    for (size_t i = 3; i < lines.size(); ++i) {
        auto c = split_csv_simple(lines[i]);
        if (c.size() < 3) continue;
        DraftItem it;
        it.pid = c[0];
        it.qty = std::stod(c[1]);
        it.unit = c[2];
        if (c.size() >= 4) it.comment = c[3];
        items.push_back(it);
    }
    return items;
}

int main(int argc, char** argv) {
    const auto PRODUCTS = (dataDir() / "products.csv").string();
    const auto BATCHES  = (dataDir() / "batches.csv").string();
    const auto EXTRAS   = (dataDir() / "extras.csv").string();


    ensure_headers(PRODUCTS, BATCHES, EXTRAS);

    ProductDB db;
    db.load(PRODUCTS);

    if (argc < 2) {
    std::cout << "Usage:\n"
            << "  ./food_tracker list\n"
            << "  ./food_tracker add-product\n"
            << "  ./food_tracker sim-batch <start YYYY-MM-DD> <days> <product> <qty><unit> [comment]\n"
            << "  ./food_tracker add-batch <start YYYY-MM-DD> <days> <product> <qty><unit> [comment]\n"
            << "  ./food_tracker add-extra <date YYYY-MM-DD> <kcal> [comment]\n"
            << "  ./food_tracker summary <start YYYY-MM-DD> <days>\n"
            << "\nDraft (multi-items):\n"
            << "  ./food_tracker draft-new <start YYYY-MM-DD> <days>\n"
            << "  ./food_tracker draft-add <product> <qty><unit> [comment]\n"
            << "  ./food_tracker draft-summary\n"
            << "  ./food_tracker draft-commit\n"
            << "  ./food_tracker draft-clear\n";
    return 0;
    }


    std::string cmd = argv[1];

    if (cmd == "list") {
        auto products = db.products; // copie
        std::sort(products.begin(), products.end(),
                    [](const Product& a, const Product& b) {
                    return a.id < b.id;
                    });

        for (const auto& p : products) {
            std::cout << std::left << std::setw(14) << p.id
                    << std::setw(22) << p.name
                    << p.kcal_per_100 << " kcal/100" << to_string(p.unit) << "\n";
        }
        return 0;
    }


    if (cmd == "add-product") {
    db.add_interactive(PRODUCTS);
    return 0;
    }
    if (cmd == "draft-new") {
    if (argc < 4) { std::cerr << "draft-new <start> <days>\n"; return 1; }
    Date start{};
    if (!parse_date_yyyy_mm_dd(argv[2], start)) { std::cerr << "Bad date\n"; return 1; }
    int days = std::stoi(argv[3]);
    if (days <= 0) { std::cerr << "days must be > 0\n"; return 1; }

    draft_init(start, days);
    std::cout << "✔ draft créé (" << format_date(start) << ", " << days << " jours)\n";
    return 0;
    }

    if (cmd == "add-extra") {
    if (argc < 4) { std::cerr << "add-extra <date> <kcal> [comment]\n"; return 1; }
    Date d{};
    if (!parse_date_yyyy_mm_dd(argv[2], d)) { std::cerr << "Bad date\n"; return 1; }
    double kcal = std::stod(argv[3]);
    std::string comment = (argc >= 5) ? join_rest_args(argc, argv, 4) : "";
    append_line(EXTRAS, format_date(d) + "," + std::to_string(kcal) + "," + comment);
    std::cout << "✔ extra ajouté\n";
    return 0;
    }
    if (cmd == "draft-add") {
    if (!draft_exists()) { std::cerr << "Aucun draft. Fais: draft-new <start> <days>\n"; return 1; }
    if (argc < 4) { std::cerr << "draft-add <product> <qty><unit> [comment]\n"; return 1; }

    std::string prod_in = argv[2];
    double qty=0.0; std::string unit;
    if (!parse_qty_unit(argv[3], qty, unit)) { std::cerr << "Bad qty/unit (ex: 700g, 250mL)\n"; return 1; }

    auto prod = db.resolve(prod_in);
    if (!prod) { std::cerr << "Produit introuvable: " << prod_in << "\n"; return 1; }

    std::string comment = (argc >= 5) ? join_rest_args(argc, argv, 4) : "";
    draft_add_line(prod->id, qty, unit, comment);
    std::cout << "✔ ajouté au draft: " << prod->id << " " << qty << unit << "\n";
    return 0;
    }
    if (cmd == "draft-summary") {
    if (!draft_exists()) { std::cerr << "Aucun draft.\n"; return 1; }

    DraftMeta meta{};
    if (!draft_read_meta(meta)) { std::cerr << "Draft invalide.\n"; return 1; }

    auto items = draft_read_items();
    if (items.empty()) { std::cout << "Draft vide (aucun item).\n"; return 0; }

    double total_week = 0.0;
    std::cout << "Draft: " << format_date(meta.start) << " sur " << meta.days << " jours\n";
    for (const auto& it : items) {
        auto p = db.get_by_id(it.pid);
        if (!p) {
            std::cout << "  ⚠ inconnu: " << it.pid << " (ignoré)\n";
            continue;
        }
        double kcal_total = it.qty * p->kcal_per_100 / 100.0;
        total_week += kcal_total;
        std::cout << "  " << p->id << " (" << p->name << "): "
                    << kcal_total << " kcal total  -> "
                    << (kcal_total / (double)meta.days) << " kcal/j\n";
    }
    std::cout << "Total draft: " << total_week << " kcal\n";
    std::cout << "Moyenne: " << (total_week / (double)meta.days) << " kcal/j\n";
    return 0;
    }

    if (cmd == "draft-commit") {
        if (!draft_exists()) { std::cerr << "Aucun draft.\n"; return 1; }

        DraftMeta meta{};
        if (!draft_read_meta(meta)) { std::cerr << "Draft invalide.\n"; return 1; }

        auto items = draft_read_items();
        if (items.empty()) { std::cerr << "Draft vide.\n"; return 1; }

        // commit: append une ligne batches.csv par item
        int k = 1;
        for (const auto& it : items) {
            // id unique : date_pid_XX
            std::string batch_id = format_date(meta.start) + "_" + it.pid + "_" + (k < 10 ? "0" : "") + std::to_string(k);
            append_line(BATCHES,
            batch_id + "," + format_date(meta.start) + "," + std::to_string(meta.days) + "," +
            it.pid + "," + std::to_string(it.qty) + "," + it.unit + "," + it.comment
            );
            k++;
        }

        draft_clear();
        std::cout << "✔ draft commit dans batches.csv (" << (k-1) << " items)\n";
        return 0;
    }

    auto do_batch = [&](bool commit) -> int {
    if (argc < 6) {
        std::cerr << (commit ? "add-batch" : "sim-batch")
                << " <start> <days> <product> <qty><unit> [comment]\n";
        return 1;
    }
    Date start{};
    if (!parse_date_yyyy_mm_dd(argv[2], start)) { std::cerr << "Bad date\n"; return 1; }
    int days = std::stoi(argv[3]);
    std::string prod_in = argv[4];

    double qty=0.0; std::string unit;
    if (!parse_qty_unit(argv[5], qty, unit)) { std::cerr << "Bad qty/unit (ex: 700g, 250ml)\n"; return 1; }

    auto prod = db.resolve(prod_in);
    if (!prod.has_value()) {
        std::cerr << "Produit introuvable: " << prod_in << " (utilise: food list / food add-product)\n";
        return 1;
    }

    // kcal calc
    double kcal_total = qty * prod->kcal_per_100 / 100.0;
    double kcal_day = kcal_total / (double)days;

    std::cout << "Produit: " << prod->name << " (" << prod->id << ")\n"
                << "Total: " << kcal_total << " kcal sur " << days << " jours -> "
                << kcal_day << " kcal/jour\n";

    if (commit) {
        std::string batch_id = format_date(start) + "_" + prod->id;
        std::string comment = (argc >= 7) ? argv[6] : "";
        append_line(BATCHES,
        batch_id + "," + format_date(start) + "," + std::to_string(days) + "," +
        prod->id + "," + std::to_string(qty) + "," + unit + "," + comment
        );
        std::cout << "✔ batch enregistré\n";
    } else {
        std::cout << "(simulation uniquement, rien n'a été enregistré)\n";
    }
    return 0;
    };
    if (cmd == "draft-clear") {
    draft_clear();
    std::cout << "✔ draft supprimé\n";
    return 0;   
    } 


    if (cmd == "sim-batch") return do_batch(false);
    if (cmd == "add-batch") return do_batch(true);

    if (cmd == "summary") {
    if (argc < 4) { std::cerr << "summary <start> <days>\n"; return 1; }
    Date start{};
    if (!parse_date_yyyy_mm_dd(argv[2], start)) { std::cerr << "Bad date\n"; return 1; }
    int days = std::stoi(argv[3]);

    auto per_day = compute_daily_kcal(db, BATCHES, EXTRAS, start, days);
    double total = 0.0;
    for (const auto& [date, kcal] : per_day) {
        std::cout << date << " : " << kcal << " kcal\n";
        total += kcal;
    }
    std::cout << "Total: " << total << " kcal\n";
    std::cout << "Moyenne: " << (total / (double)days) << " kcal/jour\n";
    return 0;
    }

    std::cerr << "Commande inconnue.\n";
    return 1;
}

#include "FoodCli.hpp"
#include "Calculator.hpp"
#include "Csv.hpp"
#include "ProductDB.hpp"
#include "Date.hpp"
#include <iostream>
#include <iomanip>
#include <filesystem>
#include <algorithm>
#include <fstream>
#include <cstdlib>
#include <cmath>
#include <sstream>

namespace {

void print_food_help() {
    std::cout << "Usage:\n"
        << "  ./DailyApp food list\n"
        << "  ./DailyApp food add-product\n"
        << "  ./DailyApp food add-extra <date YYYY-MM-DD> <kcal> [comment]\n"
        << "  ./DailyApp food history\n"
        << "\nDraft (multi-items):\n"
        << "  ./DailyApp food draft-new <start YYYY-MM-DD> <days>\n"
        << "  ./DailyApp food draft-add <product> <qty><unit> [comment]\n"
        << "  ./DailyApp food draft-summary\n"
        << "  ./DailyApp food draft-commit\n"
        << "  ./DailyApp food draft-clear\n";
}
}
namespace food {

static void ensure_headers(const std::string& products,
                           const std::string& batches,
                           const std::string& extras) {
    if (!file_exists(products)) {
    append_line(products, "id,name,unit,kcal_per_100,prot_per_100,fiber_per_100,aliases");
    }
    if (!file_exists(batches)) {
    append_line(batches, "batch_id,start_date,days,product_id,qty,unit,comment");
    }
    if (!file_exists(extras)) {
    append_line(extras, "date,kcal,prot,fiber,comment");
    }
}
static double round2(double x) {
    return std::round(x * 100.0) / 100.0;
}
static bool same_val(double a, double b) {
    return std::abs(round2(a) - round2(b)) < 1e-9;
}

struct DateRange { Date min{}; Date max{}; bool ok=false; };

static DateRange compute_available_range(const std::string& batches_csv,
                                         const std::string& extras_csv) {
    DateRange r{};
    bool have = false;
    Date minD{}, maxD{};

    // food_batches.csv: batch_id,start_date,days,product_id,qty,unit,comment
    if (file_exists(batches_csv)) {
        auto lines = read_lines(batches_csv);
        for (size_t i = 1; i < lines.size(); ++i) {
            auto c = split_csv_simple(lines[i]);
            if (c.size() < 3) continue;

            Date start{};
            if (!parse_date_yyyy_mm_dd(c[1], start)) continue;

            int days = 0;
            try { days = std::stoi(c[2]); } catch (...) { continue; }
            if (days <= 0) continue;

            Date end = add_days(start, days - 1);

            if (!have) { minD = start; maxD = end; have = true; }
            else {
                if (start < minD) minD = start;
                if (maxD < end)   maxD = end;
            }
        }
    }

    // food_extras.csv: date,kcal,prot,comment
    if (file_exists(extras_csv)) {
        auto lines = read_lines(extras_csv);
        for (size_t i = 1; i < lines.size(); ++i) {
            auto c = split_csv_simple(lines[i]);
            if (c.size() < 1) continue;

            Date d{};
            if (!parse_date_yyyy_mm_dd(c[0], d)) continue;

            if (!have) { minD = d; maxD = d; have = true; }
            else {
                if (d < minD) minD = d;
                if (maxD < d) maxD = d;
            }
        }
    }

    r.ok = have;
    r.min = minD;
    r.max = maxD;
    return r;
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
    return std::filesystem::path(DAILYAPP_DATA_DIR);
}

static std::string join_rest_args(std::span<const std::string_view> args, size_t start_idx) {
    std::string out;
    for (size_t i = start_idx; i < args.size(); ++i) {
        if (i > start_idx) out += " ";
        out += std::string(args[i]);
    }
    return out;
}


static std::filesystem::path draftPath() {
    return dataDir() / "draft.csv";
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
static int runFoodHistoryPlot() {
    const std::filesystem::path root = std::filesystem::path(DAILYAPP_ROOT_DIR);
    const std::filesystem::path csv  = dataDir() / "food_history.csv";
    const std::filesystem::path out  = dataDir() / "food_history.png";
    const std::filesystem::path py   = root / "analytics" / "food_history.py";

    if (!std::filesystem::exists(py)) {
        std::cerr << "[plot] Script not found: " << py << "\n";
        return 127;
    }

    std::ostringstream cmd;
    cmd << "python3 "
        << "\"" << py.string() << "\""
        << " --csv " << "\"" << csv.string() << "\""
        << " --out " << "\"" << out.string() << "\"";

    int rc = std::system(cmd.str().c_str());
    if (rc != 0)
        std::cerr << "[plot] analytics failed (exit code " << rc << ")\n";
    return rc;
}

static int rebuild_food_history_csv(ProductDB& db,
                                    const std::string& batches,
                                    const std::string& extras,
                                    const std::string& out_csv)
{
    auto range = compute_available_range(batches, extras);
    if (!range.ok) {
        // pas d'erreur fatale : on peut juste vider le cache ou ne rien faire
        // je préfère ne rien faire et informer.
        std::cerr << "No data found in food_batches.csv / food_extras.csv.\n";
        return 1;
    }

    int days = days_between_inclusive(range.min, range.max);
    if (days <= 0) {
        std::cerr << "Invalid computed date range.\n";
        return 2;
    }

    auto per = compute_daily_kcal_and_prot_and_fiber(db, batches, extras, range.min, days);

    std::ofstream out(out_csv, std::ios::trunc);
    if (!out) {
        std::cerr << "Cannot write: " << out_csv << "\n";
        return 3;
    }

    out << "date,kcal,protein,fiber\n";
    for (const auto& [date_str, kcal] : per.kcal) {
        double prot  = per.prot[date_str];
        double fiber = per.fiber[date_str];
        out << date_str << "," << kcal << "," << prot << "," << fiber << "\n";
    }
    return 0;
}
static int print_grouped_history_from_csv(const std::string& csv_path) {
    if (!file_exists(csv_path)) {
        std::cerr << "Missing cache: " << csv_path << "\n"
                  << "Run a write command (add-extra / draft-commit) first.\n";
        return 1;
    }

    auto lines = read_lines(csv_path);
    if (lines.size() < 2) {
        std::cout << "Historique vide.\n";
        return 0;
    }

    bool started = false;
    std::string grp_start, grp_end;
    double grp_kcal = 0.0, grp_prot = 0.0, grp_fiber = 0.0;

    auto flush_group = [&]() {
        if (!started) return;
        if (same_val(grp_kcal, 0.0) && same_val(grp_prot, 0.0) && same_val(grp_fiber, 0.0)) return;

        if (grp_start == grp_end) {
            std::cout << grp_start << " : " << round2(grp_kcal)
                      << " kcal | " << round2(grp_prot) << " g prot | " << round2(grp_fiber) << " g fiber\n";
        } else {
            std::cout << grp_start << " -> " << grp_end << " : " << round2(grp_kcal)
                      << " kcal | " << round2(grp_prot) << " g prot | " << round2(grp_fiber) << " g fiber\n";
        }
    };

    bool have_prev = false;
    Date prev{};

    for (size_t i = 1; i < lines.size(); ++i) { // skip header
        auto c = split_csv_simple(lines[i]);
        if (c.size() < 4) continue;

        const std::string& date_str = c[0];
        double kcal  = std::stod(c[1]);
        double prot  = std::stod(c[2]);
        double fiber = std::stod(c[3]);

        Date cur{};
        if (!parse_date_yyyy_mm_dd(date_str, cur)) continue;

        bool consecutive = false;
        if (have_prev) {
            Date expected = add_days(prev, 1);
            consecutive = (cur == expected);
        }

        if (!started) {
            started = true;
            grp_start = grp_end = date_str;
            grp_kcal = kcal; grp_prot = prot; grp_fiber = fiber;
        } else {
            if (consecutive && same_val(kcal, grp_kcal) && same_val(prot, grp_prot) && same_val(fiber, grp_fiber)) {
                grp_end = date_str;
            } else {
                flush_group();
                grp_start = grp_end = date_str;
                grp_kcal = kcal; grp_prot = prot; grp_fiber = fiber;
            }
        }

        prev = cur;
        have_prev = true;
    }

    flush_group();
    return 0;
}
int run(std::span<const std::string_view> args) {
    if (args.empty() || args[0] == "--help" || args[0] == "-h") {
        print_food_help();
        return 0;
    }

    const std::string_view cmd = args[0];

    // --- chemins centralisés ---
    const std::string PRODUCTS = (dataDir() / "food_products.csv").string();
    const std::string BATCHES  = (dataDir() / "food_batches.csv").string();
    const std::string EXTRAS   = (dataDir() / "food_extras.csv").string();

    ensure_headers(PRODUCTS, BATCHES, EXTRAS);

    ProductDB db;
    db.load(PRODUCTS);

    if (cmd == "list") {
        auto products = db.products; // copie
        std::sort(products.begin(), products.end(),
                  [](const Product& a, const Product& b) { return a.id < b.id; });

        for (const auto& p : products) {
            std::cout << std::left
                      << std::setw(14) << p.id
                      << std::setw(22) << p.name
                      << std::setw(8)  << p.kcal_per_100
                      << std::setw(12) << ("kcal/100" + to_string(p.unit))
                      << std::setw(8)  << p.prot_per_100
                      << ("g/100" + to_string(p.unit))
                      << std::setw(12) << p.fiber_per_100
                      << ("g/100" + to_string(p.unit))
                      << "\n";
        }
        return 0;
    }

    if (cmd == "add-product") {
        db.add_interactive(PRODUCTS);
        return 0;
    }

    if (cmd == "draft-new") {
        if (args.size() != 3) { std::cerr << "draft-new <start> <days>\n"; return 1; }
        Date start{};
        if (!parse_date_yyyy_mm_dd(std::string(args[1]), start)) { std::cerr << "Bad date\n"; return 1; }
        int days = 0;
        try { days = std::stoi(std::string(args[2])); } catch (...) { return 1; }
        if (days <= 0) { std::cerr << "days must be > 0\n"; return 1; }

        draft_init(start, days);
        std::cout << "✔ draft créé (" << format_date(start) << ", " << days << " jours)\n";
        return 0;
    }

    if (cmd == "add-extra") {
        // add-extra <date> <kcal> [comment...]
        if (args.size() < 3) { std::cerr << "add-extra <date> <kcal> [comment]\n"; return 1; }

        Date d{};
        if (!parse_date_yyyy_mm_dd(std::string(args[1]), d)) { std::cerr << "Bad date\n"; return 1; }

        double kcal = 0.0;
        try { kcal = std::stod(std::string(args[2])); } catch (...) { return 1; }

        std::string comment = (args.size() >= 4) ? join_rest_args(args, 3) : "";

        // food_extras.csv: date,kcal,prot,fiber,comment
        append_line(EXTRAS, format_date(d) + "," + std::to_string(kcal) + ",0,0," + comment);

        std::cout << "✔ extra ajouté\n";
        const auto HISTORY_CSV = (dataDir() / "food_history.csv").string();
        rebuild_food_history_csv(db, BATCHES, EXTRAS, HISTORY_CSV);
        return 0;
    }

    if (cmd == "draft-add") {
        // draft-add <product> <qty><unit> [comment...]
        if (!draft_exists()) { std::cerr << "Aucun draft. Fais: draft-new <start> <days>\n"; return 1; }
        if (args.size() < 3) { std::cerr << "draft-add <product> <qty><unit> [comment]\n"; return 1; }

        std::string prod_in = std::string(args[1]);
        double qty = 0.0;
        std::string unit;
        if (!parse_qty_unit(std::string(args[2]), qty, unit)) { std::cerr << "Bad qty/unit (ex: 700g, 250mL)\n"; return 1; }

        auto prod = db.resolve(prod_in);
        if (!prod) { std::cerr << "Produit introuvable: " << prod_in << "\n"; return 1; }

        std::string comment = (args.size() >= 4) ? join_rest_args(args, 3) : "";
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

        double total_week = 0.0, prot_week = 0.0, fiber_week = 0.0;

        std::cout << "Draft: " << format_date(meta.start) << " sur " << meta.days << " jours\n";
        for (const auto& it : items) {
            auto p = db.get_by_id(it.pid);
            if (!p) {
                std::cout << "  ⚠ inconnu: " << it.pid << " (ignoré)\n";
                continue;
            }
            double kcal_total  = it.qty * p->kcal_per_100 / 100.0;
            double prot_total  = it.qty * p->prot_per_100 / 100.0;
            double fiber_total = it.qty * p->fiber_per_100 / 100.0;

            total_week += kcal_total;
            prot_week  += prot_total;
            fiber_week += fiber_total;

            std::cout << "  " << p->id << " (" << p->name << "): "
                      << kcal_total << " kcal total  -> "
                      << (kcal_total / (double)meta.days) << " kcal/j ; "
                      << prot_total << " prot total  -> "
                      << (prot_total / (double)meta.days) << " g prot/j ; "
                      << fiber_total << " fiber total  -> "
                      << (fiber_total / (double)meta.days) << " g fiber/j\n";
        }

        std::cout << "Total draft: " << total_week << " kcal ; " << prot_week << " g prot ; " << fiber_week << " g fiber\n";
        std::cout << "Moyenne: " << (total_week / (double)meta.days) << " kcal/j ; "
                  << (prot_week / (double)meta.days) << " g prot/j ; "
                  << (fiber_week / (double)meta.days) << " g fiber/j\n";
        return 0;
    }

    if (cmd == "draft-commit") {
        if (!draft_exists()) { std::cerr << "Aucun draft.\n"; return 1; }

        DraftMeta meta{};
        if (!draft_read_meta(meta)) { std::cerr << "Draft invalide.\n"; return 1; }

        auto items = draft_read_items();
        if (items.empty()) { std::cerr << "Draft vide.\n"; return 1; }

        int k = 1;
        for (const auto& it : items) {
            std::string batch_id = format_date(meta.start) + "_" + it.pid + "_" +
                                   (k < 10 ? "0" : "") + std::to_string(k);

            append_line(BATCHES,
                batch_id + "," + format_date(meta.start) + "," + std::to_string(meta.days) + "," +
                it.pid + "," + std::to_string(it.qty) + "," + it.unit + "," + it.comment
            );
            k++;
        }

        const auto HISTORY_CSV = (dataDir() / "food_history.csv").string();
        rebuild_food_history_csv(db, BATCHES, EXTRAS, HISTORY_CSV);

        draft_clear();
        std::cout << "✔ draft commit dans food_batches.csv (" << (k-1) << " items)\n";
        return 0;
    }

    if (cmd == "draft-clear") {
        draft_clear();
        std::cout << "✔ draft supprimé\n";
        return 0;
    }

    if (cmd == "history") {
        const auto HISTORY_CSV = (dataDir() / "food_history.csv").string();

        int prc = print_grouped_history_from_csv(HISTORY_CSV);
        if (prc != 0) return prc;

        int rc = runFoodHistoryPlot();
        if (rc != 0) {
            return rc; // comme tu voulais
        }

        std::cout << "✔ plot updated: " << (dataDir() / "food_history.png") << "\n";
        return 0;
    }

    std::cerr << "Unknown food command: " << cmd << "\n";
    print_food_help();
    return 2;
}

} // namespace food

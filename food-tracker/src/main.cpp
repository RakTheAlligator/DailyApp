#include "ProductDB.hpp"
#include "Calculator.hpp"
#include "Csv.hpp"
#include "Date.hpp"
#include <iostream>
#include <iomanip>
#include <filesystem>
#include <algorithm>
#include <fstream>
#include <cstdlib>
#include <unistd.h>   // readlink
#include <limits.h>   // PATH_MAX
#include <cmath>
#include <sstream>

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
static int runFoodHistoryPlot() {
    const std::filesystem::path root = std::filesystem::path(DAILYAPP_ROOT_DIR);
    const std::filesystem::path csv  = std::filesystem::path(FOOD_TRACKER_DATA_DIR) / "food_history.csv";
    const std::filesystem::path out  = std::filesystem::path(FOOD_TRACKER_DATA_DIR) / "food_history.png";
    const std::filesystem::path py   = root / "analytics" / "food_history.py"; // script global DailyApp/analytics

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

static void usage() {
    std::cout << "Usage:\n"
        << "  ./food_tracker food list\n"
        << "  ./food_tracker food add-product\n"
        << "  ./food_tracker food sim-batch <start YYYY-MM-DD> <days> <product> <qty><unit> [comment]\n"
        << "  ./food_tracker food add-batch <start YYYY-MM-DD> <days> <product> <qty><unit> [comment]\n"
        << "  ./food_tracker food add-extra <date YYYY-MM-DD> <kcal> [comment]\n"
        << "  ./food_tracker food history\n"
        << "\nDraft (multi-items):\n"
        << "  ./food_tracker food draft-new <start YYYY-MM-DD> <days>\n"
        << "  ./food_tracker food draft-add <product> <qty><unit> [comment]\n"
        << "  ./food_tracker food draft-summary\n"
        << "  ./food_tracker food draft-commit\n"
        << "  ./food_tracker food draft-clear\n";
}

int main(int argc, char** argv) {
    const auto PRODUCTS = (dataDir() / "food_products.csv").string();
    const auto BATCHES  = (dataDir() / "food_batches.csv").string();
    const auto EXTRAS   = (dataDir() / "food_extras.csv").string();


    ensure_headers(PRODUCTS, BATCHES, EXTRAS);

    ProductDB db;
    db.load(PRODUCTS);

    if (argc < 2) { usage(); return 1; }

    std::string top = argv[1];
    if (top == "-h" || top == "--help" || top == "help") { usage(); return 0; }

    if (top != "food") {
        std::cerr << "Commande inconnue: " << top << "\n";
        usage();
        return 2;
    }

    if (argc < 3) { usage(); return 1; }
    std::string cmd = argv[2];
    const int ARG0 = 3;

    if (cmd == "list") {
        auto products = db.products; // copie
        std::sort(products.begin(), products.end(),
                    [](const Product& a, const Product& b) {
                    return a.id < b.id;
                    });

        for (const auto& p : products) {
            std::cout << std::left
                    << std::setw(14) << p.id
                    << std::setw(22) << p.name
                    << std::setw(8)  << p.kcal_per_100
                    << std::setw(12) << ("kcal/100" + to_string(p.unit))
                    << std::setw(8)  << p.prot_per_100
                    << ("g/100" + to_string(p.unit))
                    << std::setw(8)  << p.fiber_per_100
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
    if (argc < ARG0 + 2) { std::cerr << "draft-new <start> <days>\n"; return 1; }
    Date start{};
    if (!parse_date_yyyy_mm_dd(argv[ARG0], start)) { std::cerr << "Bad date\n"; return 1; }
    int days = std::stoi(argv[ARG0 + 1]);
    if (days <= 0) { std::cerr << "days must be > 0\n"; return 1; }

    draft_init(start, days);
    std::cout << "✔ draft créé (" << format_date(start) << ", " << days << " jours)\n";
    return 0;
    }

    if (cmd == "add-extra") {
        if (argc < ARG0 + 2) { std::cerr << "add-extra <date> <kcal> [comment]\n"; return 1; }
        Date d{};
        if (!parse_date_yyyy_mm_dd(argv[ARG0], d)) { std::cerr << "Bad date\n"; return 1; }

        double kcal = std::stod(argv[ARG0 + 1]);
        std::string comment = (argc >= ARG0 + 3) ? join_rest_args(argc, argv, ARG0 + 2) : "";

        // food_extras.csv: date,kcal,prot,fiber,comment  (prot=0 pour l’instant)
        append_line(EXTRAS, format_date(d) + "," + std::to_string(kcal) + ",0,0," + comment);

        std::cout << "✔ extra ajouté\n";
        return 0;
    }

    if (cmd == "draft-add") {
    if (!draft_exists()) { std::cerr << "Aucun draft. Fais: draft-new <start> <days>\n"; return 1; }
    if (argc < ARG0 + 2) { std::cerr << "draft-add <product> <qty><unit> [comment]\n"; return 1; }

    std::string prod_in = argv[ARG0];
    double qty=0.0; std::string unit;
    if (!parse_qty_unit(argv[ARG0 + 1], qty, unit)) { std::cerr << "Bad qty/unit (ex: 700g, 250mL)\n"; return 1; }
    auto prod = db.resolve(prod_in);
    if (!prod) { std::cerr << "Produit introuvable: " << prod_in << "\n"; return 1; }

    std::string comment = (argc >= ARG0 + 3) ? join_rest_args(argc, argv, ARG0 + 2) : "";
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
    double prot_week = 0.0;
    double fiber_week = 0.0;

    std::cout << "Draft: " << format_date(meta.start) << " sur " << meta.days << " jours\n";
    for (const auto& it : items) {
        auto p = db.get_by_id(it.pid);
        if (!p) {
            std::cout << "  ⚠ inconnu: " << it.pid << " (ignoré)\n";
            continue;
        }
        double kcal_total = it.qty * p->kcal_per_100 / 100.0;
        double prot_total = it.qty * p->prot_per_100 / 100.0;
        double fiber_total = it.qty * p->fiber_per_100 / 100.0;

        total_week += kcal_total;
        prot_week += prot_total;
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
    std::cout << "Moyenne: " << (total_week / (double)meta.days) << " kcal/j ; " << (prot_week / (double)meta.days) << " g prot/j ; " << (fiber_week / (double)meta.days) << " g fiber/j\n";
    return 0;
    }

    if (cmd == "draft-commit") {
        if (!draft_exists()) { std::cerr << "Aucun draft.\n"; return 1; }

        DraftMeta meta{};
        if (!draft_read_meta(meta)) { std::cerr << "Draft invalide.\n"; return 1; }

        auto items = draft_read_items();
        if (items.empty()) { std::cerr << "Draft vide.\n"; return 1; }

        // commit: append une ligne food_batches.csv par item
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
        std::cout << "✔ draft commit dans food_batches.csv (" << (k-1) << " items)\n";
        return 0;
    }

    auto do_batch = [&](bool commit) -> int {
        if (argc < ARG0 + 4) {
            std::cerr << (commit ? "add-batch" : "sim-batch")
                    << " <start> <days> <product> <qty><unit> [comment]\n";
            return 1;
        }

        Date start{};
        if (!parse_date_yyyy_mm_dd(argv[ARG0], start)) { std::cerr << "Bad date\n"; return 1; }

        int days = std::stoi(argv[ARG0 + 1]);
        std::string prod_in = argv[ARG0 + 2];

        double qty=0.0; std::string unit;
        if (!parse_qty_unit(argv[ARG0 + 3], qty, unit)) {
            std::cerr << "Bad qty/unit (ex: 700g, 250mL)\n"; return 1;
        }

        auto prod = db.resolve(prod_in);
        if (!prod.has_value()) {
            std::cerr << "Produit introuvable: " << prod_in << " (utilise: food list / food add-product)\n";
            return 1;
        }

        double kcal_total = qty * prod->kcal_per_100 / 100.0;
        double kcal_day = kcal_total / (double)days;

        std::cout << "Produit: " << prod->name << " (" << prod->id << ")\n"
                << "Total: " << kcal_total << " kcal sur " << days << " jours -> "
                << kcal_day << " kcal/jour\n";

        if (commit) {
            std::string batch_id = format_date(start) + "_" + prod->id;
            std::string comment = (argc >= ARG0 + 5) ? join_rest_args(argc, argv, ARG0 + 4) : "";
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

    if (cmd == "history") {
        // 1) Determine full available range (min..max)
        auto range = compute_available_range(BATCHES, EXTRAS);
        if (!range.ok) {
            std::cerr << "No data found in food_batches.csv / food_extras.csv.\n";
            return 1;
        }

        int days = days_between_inclusive(range.min, range.max);
        if (days <= 0) {
            std::cerr << "Invalid computed date range.\n";
            return 1;
        }

        // 2) Compute daily kcal/prot over whole range
        auto per = compute_daily_kcal_and_prot_and_fiber(db, BATCHES, EXTRAS, range.min, days);

        // 3) Write/overwrite history_food.csv
        const auto HISTORY_CSV = (dataDir() / "food_history.csv").string();
        std::ofstream out(HISTORY_CSV, std::ios::trunc);
        if (!out) {
            std::cerr << "Cannot write: " << HISTORY_CSV << "\n";
            return 1;
        }
        out << "date,kcal,protein,fiber\n";
        // 4) Print grouped consecutive ranges with identical values
        bool started = false;
        std::string grp_start, grp_end;
        double grp_kcal = 0.0, grp_prot = 0.0, grp_fiber = 0.0;

        auto flush_group = [&]() {
            if (!started) return;

            // Skip pure zero segments (after rounding)
            if (same_val(grp_kcal, 0.0) && same_val(grp_prot, 0.0)) {
                return;
            }

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
        double total_kcal = 0.0, total_prot = 0.0, total_fiber = 0.0;

        // per.kcal is assumed to be ordered (std::map). If not, tell me and we sort.
        for (const auto& [date_str, kcal] : per.kcal) {
            double prot = per.prot[date_str];
            double fiber = per.fiber[date_str];
            // CSV
            out << date_str << "," << kcal << "," << prot << "," << fiber << "\n";

            total_kcal += kcal;
            total_prot += prot;
            total_fiber += fiber;

            // grouping logic
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
                grp_kcal = kcal;
                grp_prot = prot;
                grp_fiber = fiber;

            } else {
                if (consecutive && same_val(kcal, grp_kcal) && same_val(prot, grp_prot) && same_val(fiber, grp_fiber)) {
                    grp_end = date_str;
                } else {
                    flush_group();
                    grp_start = grp_end = date_str;
                    grp_kcal = kcal;
                    grp_prot = prot;
                    grp_fiber = fiber;
                }
            }

            prev = cur;
            have_prev = true;
        }

        out.close();
        flush_group();

        // 5) Plot entire history from history_food.csv
        runFoodHistoryPlot();
        std::cout << "✔ plot updated: " << (dataDir() / "food_history.png") << "\n";

        return 0;
    }
    std::cerr << "Commande inconnue.\n";
    return 1;
}

#include "ProductDB.hpp"
#include "Calculator.hpp"
#include "Csv.hpp"
#include "Date.hpp"
#include <iostream>
#include <iomanip>
#include <filesystem>


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

int main(int argc, char** argv) {
  const auto PRODUCTS = (dataDir() / "products.csv").string();
  const auto BATCHES  = (dataDir() / "batches.csv").string();
  const auto EXTRAS   = (dataDir() / "extras.csv").string();


  ensure_headers(PRODUCTS, BATCHES, EXTRAS);

  ProductDB db;
  db.load(PRODUCTS);

  if (argc < 2) {
    std::cout << "Usage:\n"
              << "  food list\n"
              << "  food add-product\n"
              << "  food sim-batch <start YYYY-MM-DD> <days> <product> <qty><unit> [comment]\n"
              << "  food add-batch <start YYYY-MM-DD> <days> <product> <qty><unit> [comment]\n"
              << "  food add-extra <date YYYY-MM-DD> <kcal> [comment]\n"
              << "  food summary <start YYYY-MM-DD> <days>\n";
    return 0;
  }

  std::string cmd = argv[1];

  if (cmd == "list") {
    for (const auto& p : db.products) {
      std::cout << std::left << std::setw(12) << p.id
                << std::setw(22) << p.name
                << p.kcal_per_100 << " kcal/100" << to_string(p.unit) << "\n";
    }
    return 0;
  }

  if (cmd == "add-product") {
    db.add_interactive(PRODUCTS);
    return 0;
  }

  if (cmd == "add-extra") {
    if (argc < 4) { std::cerr << "add-extra <date> <kcal> [comment]\n"; return 1; }
    Date d{};
    if (!parse_date_yyyy_mm_dd(argv[2], d)) { std::cerr << "Bad date\n"; return 1; }
    double kcal = std::stod(argv[3]);
    std::string comment = (argc >= 5) ? argv[4] : "";
    append_line(EXTRAS, format_date(d) + "," + std::to_string(kcal) + "," + comment);
    std::cout << "✔ extra ajouté\n";
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

#include "ProductDB.hpp"
#include "Csv.hpp"
#include <iostream>
#include <algorithm>

static std::string lower(std::string s) {
  std::transform(s.begin(), s.end(), s.begin(),
                 [](unsigned char c){ return (char)std::tolower(c); });
  return s;
}

static Unit parse_unit(const std::string& s) {
  auto t = lower(trim(s));
  if (t == "g") return Unit::G;
  if (t == "ml" || t == "mL") return Unit::ML;
  // défaut
  return Unit::G;
}

static bool contains_ci(const std::string& hay, const std::string& needle) {
  return lower(hay).find(lower(needle)) != std::string::npos;
}

bool ProductDB::load(const std::string& path) {
  products.clear(); by_id.clear(); by_token.clear();
  auto lines = read_lines(path);
  if (lines.empty()) return false;

  // skip header
  for (size_t i = 1; i < lines.size(); ++i) {
    auto cols = split_csv_simple(lines[i]);
    if (cols.size() < 6) continue;

    Product p;
    p.id = cols[0];
    p.name = cols[1];
    p.unit = parse_unit(cols[2]);
    p.kcal_per_100 = std::stod(cols[3]);
    p.prot_per_100 = std::stod(cols[4]);
    p.fiber_per_100 = std::stod(cols[5]);
    if (cols.size() >= 7) p.aliases_raw = cols[6];

    size_t idx = products.size();
    products.push_back(p);
    by_id[lower(p.id)] = idx;

    // tokens: id + name + aliases (séparés par |)
    by_token[lower(p.id)] = idx;
    by_token[lower(p.name)] = idx;
    if (!p.aliases_raw.empty()) {
      std::string a = p.aliases_raw;
      size_t start = 0;
      while (true) {
        size_t pos = a.find('|', start);
        std::string tok = (pos == std::string::npos) ? a.substr(start) : a.substr(start, pos-start);
        tok = lower(trim(tok));
        if (!tok.empty()) by_token[tok] = idx;
        if (pos == std::string::npos) break;
        start = pos + 1;
      }
    }
  }
  return !products.empty();
}

std::optional<Product> ProductDB::get_by_id(const std::string& id) const {
  auto it = by_id.find(lower(id));
  if (it == by_id.end()) return std::nullopt;
  return products[it->second];
}

std::vector<Product> ProductDB::search(const std::string& query) const {
  std::vector<Product> out;
  for (const auto& p : products) {
    if (contains_ci(p.id, query) || contains_ci(p.name, query) || contains_ci(p.aliases_raw, query))
      out.push_back(p);
  }
  return out;
}

std::optional<Product> ProductDB::resolve(const std::string& user_input) const {
  auto key = lower(trim(user_input));
  if (key.empty()) return std::nullopt;

  // match direct token
  auto it = by_token.find(key);
  if (it != by_token.end()) return products[it->second];

  // fallback: recherche "contains" si unique
  auto matches = search(key);
  if (matches.size() == 1) return matches[0];
  return std::nullopt;
}

bool ProductDB::add_interactive(const std::string& path) {
  Product p;
  std::cout << "id (ex: bread): ";
  std::getline(std::cin, p.id);
  p.id = trim(p.id);

  std::cout << "name (ex: Pain de mie): ";
  std::getline(std::cin, p.name);
  p.name = trim(p.name);

  std::string unit;
  std::cout << "unit (g ou mL): ";
  std::getline(std::cin, unit);
  p.unit = parse_unit(unit);

  std::string kcal;
  std::cout << "kcal_per_100 (ex: 265): ";
  std::getline(std::cin, kcal);
  p.kcal_per_100 = std::stod(trim(kcal));

  std::string prot;
  std::cout << "prot_per_100 (ex: 10): ";
  std::getline(std::cin, prot);
  p.prot_per_100 = std::stod(trim(prot));

  std::string fiber;
  std::cout << "fiber_per_100 (ex: 2): ";
  std::getline(std::cin, fiber);
  p.fiber_per_100 = std::stod(trim(fiber));

  std::cout << "aliases (optionnel, séparés par |): ";
  std::getline(std::cin, p.aliases_raw);
  p.aliases_raw = trim(p.aliases_raw);

  // append
  append_line(path,
    p.id + "," + p.name + "," + to_string(p.unit) + "," + std::to_string(p.kcal_per_100) + "," + std::to_string(p.prot_per_100) + "," + std::to_string(p.fiber_per_100) + "," + p.aliases_raw
  );
  return true;
}

#pragma once
#include <string>

enum class Unit { G, ML };

inline std::string to_string(Unit u) {
  return (u == Unit::G) ? "g" : "mL";
}

struct Product {
  std::string id;
  std::string name;
  Unit unit{};
  double kcal_per_100 = 0.0;   // kcal / 100g ou kcal / 100mL
  double prot_per_100 = 0.0;
  std::string aliases_raw;     // "pain|mie|toast"
};

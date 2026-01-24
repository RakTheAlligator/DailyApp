#pragma once
#include "Date.hpp"
#include <map>
#include <string>

struct ProductDB;

struct DayMacros {
  std::map<std::string, double> kcal;
  std::map<std::string, double> prot;
};

DayMacros compute_daily_kcal_and_prot(
  const ProductDB& db,
  const std::string& batches_csv,
  const std::string& extras_csv,
  const Date& start,
  int days
);

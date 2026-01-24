#pragma once
#include "Date.hpp"
#include <map>
#include <string>

struct ProductDB;

// retourne kcal_total par jour sur l’intervalle [start, start+days-1]
std::map<std::string, double> compute_daily_kcal(
  const ProductDB& db,
  const std::string& batches_csv,
  const std::string& extras_csv,
  const Date& start,
  int days
);

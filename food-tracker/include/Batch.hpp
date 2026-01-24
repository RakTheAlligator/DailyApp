#pragma once
#include "Date.hpp"
#include "Product.hpp"
#include <string>

struct Batch {
  std::string batch_id;
  Date start{};
  int days = 0;
  std::string product_id;
  double qty = 0.0;
  Unit unit{};
  std::string comment;
};

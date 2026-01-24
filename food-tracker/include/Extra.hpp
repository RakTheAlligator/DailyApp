#pragma once
#include "Date.hpp"
#include <string>

struct Extra {
  Date date{};
  double kcal = 0.0;
  std::string comment;
};

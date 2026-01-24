#pragma once
#include "Product.hpp"
#include <unordered_map>
#include <optional>
#include <vector>

struct ProductDB {
  std::vector<Product> products;
  std::unordered_map<std::string, size_t> by_id;     // id -> index
  std::unordered_map<std::string, size_t> by_token;  // alias/name token -> index

  bool load(const std::string& path);
  bool add_interactive(const std::string& path); // ajoute + append dans products.csv

  std::optional<Product> get_by_id(const std::string& id) const;
  std::vector<Product> search(const std::string& query) const; // simple contains
  std::optional<Product> resolve(const std::string& user_input) const; // id ou alias unique
};

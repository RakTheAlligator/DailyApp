#include "Csv.hpp"
#include <fstream>
#include <sstream>
#include <algorithm>
#include <cctype>
#include <filesystem>

std::string trim(std::string s) {
  auto not_space = [](unsigned char c){ return !std::isspace(c); };
  s.erase(s.begin(), std::find_if(s.begin(), s.end(), not_space));
  s.erase(std::find_if(s.rbegin(), s.rend(), not_space).base(), s.end());
  return s;
}

std::vector<std::string> split_csv_simple(const std::string& line) {
  // CSV minimal: pas de guillemets. Suffit pour ton usage.
  std::vector<std::string> out;
  std::stringstream ss(line);
  std::string tok;
  while (std::getline(ss, tok, ',')) out.push_back(trim(tok));
  return out;
}

std::vector<std::string> read_lines(const std::string& path) {
  std::ifstream f(path);
  std::vector<std::string> lines;
  if (!f) return lines;
  std::string line;
  while (std::getline(f, line)) {
    line = trim(line);
    if (!line.empty()) lines.push_back(line);
  }
  return lines;
}

void append_line(const std::string& path, const std::string& line) {
  std::ofstream f(path, std::ios::app);
  f << line << "\n";
}

bool file_exists(const std::string& path) {
  return std::filesystem::exists(path);
}

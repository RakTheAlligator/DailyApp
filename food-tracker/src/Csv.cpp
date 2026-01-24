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
  // Assure que le dossier parent existe (utile si path = ".../data/batches.csv")
  const auto parent = std::filesystem::path(path).parent_path();
  if (!parent.empty()) std::filesystem::create_directories(parent);

  bool need_leading_newline = false;

  if (std::filesystem::exists(path)) {
    std::ifstream in(path, std::ios::binary);
    in.seekg(0, std::ios::end);
    const auto size = in.tellg();
    if (size > 0) {
      in.seekg(-1, std::ios::end);
      char last = '\0';
      in.get(last);
      if (last != '\n') need_leading_newline = true;
    }
  }

  std::ofstream out(path, std::ios::app | std::ios::binary);
  if (need_leading_newline) out << "\n";
  out << line << "\n";
}


bool file_exists(const std::string& path) {
  return std::filesystem::exists(path);
}

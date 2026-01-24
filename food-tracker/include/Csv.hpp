#pragma once
#include <string>
#include <vector>

std::string trim(std::string s);
std::vector<std::string> split_csv_simple(const std::string& line);

// read all non-empty lines (excluding header optionally)
std::vector<std::string> read_lines(const std::string& path);
void append_line(const std::string& path, const std::string& line);
bool file_exists(const std::string& path);

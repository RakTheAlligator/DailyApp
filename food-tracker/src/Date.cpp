#include "Date.hpp"
#include <sstream>
#include <iomanip>
#include <stdexcept>

static int to_int(const std::string& s) {
  return std::stoi(s);
}

bool parse_date_yyyy_mm_dd(const std::string& s, Date& out) {
  // format strict YYYY-MM-DD
  if (s.size() != 10 || s[4] != '-' || s[7] != '-') return false;
  try {
    out.y = to_int(s.substr(0,4));
    out.m = to_int(s.substr(5,2));
    out.d = to_int(s.substr(8,2));
  } catch (...) { return false; }
  if (out.m < 1 || out.m > 12) return false;
  if (out.d < 1 || out.d > 31) return false;
  return true;
}

std::string format_date(const Date& dt) {
  std::ostringstream oss;
  oss << std::setw(4) << std::setfill('0') << dt.y << "-"
      << std::setw(2) << std::setfill('0') << dt.m << "-"
      << std::setw(2) << std::setfill('0') << dt.d;
  return oss.str();
}

// Simple Gregorian date add (ok pour usage perso) via std::tm
#include <ctime>

static std::tm to_tm(const Date& dt) {
  std::tm t{};
  t.tm_year = dt.y - 1900;
  t.tm_mon  = dt.m - 1;
  t.tm_mday = dt.d;
  t.tm_hour = 12; // éviter DST edge
  return t;
}
static Date from_tm(const std::tm& t) {
  return Date{t.tm_year + 1900, t.tm_mon + 1, t.tm_mday};
}

Date add_days(const Date& dt, int delta) {
  std::tm t = to_tm(dt);
  std::time_t tt = std::mktime(&t);
  tt += static_cast<long long>(delta) * 24LL * 3600LL;
  std::tm out{};
  localtime_r(&tt, &out);
  return from_tm(out);
}

bool operator<(const Date& a, const Date& b) {
  if (a.y != b.y) return a.y < b.y;
  if (a.m != b.m) return a.m < b.m;
  return a.d < b.d;
}
bool operator==(const Date& a, const Date& b) {
  return a.y==b.y && a.m==b.m && a.d==b.d;
}

int days_between_inclusive(const Date& start, const Date& end) {
  // supposé start <= end
  int count = 0;
  Date cur = start;
  while (!(end < cur)) { // cur <= end
    count++;
    cur = add_days(cur, 1);
  }
  return count;
}

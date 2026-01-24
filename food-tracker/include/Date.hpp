#pragma once
#include <string>
#include <cstdint>

struct Date {
  int y=0, m=0, d=0;
};

bool parse_date_yyyy_mm_dd(const std::string& s, Date& out);
std::string format_date(const Date& dt);
Date add_days(const Date& dt, int delta);
int days_between_inclusive(const Date& start, const Date& end); // start..end
bool operator<(const Date& a, const Date& b);
bool operator==(const Date& a, const Date& b);

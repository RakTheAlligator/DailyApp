#include "Calculator.hpp"
#include "ProductDB.hpp"
#include "Csv.hpp"
#include "Date.hpp"
#include <iostream>

static bool in_range(const Date& d, const Date& start, const Date& end) {
  return !(d < start) && !(end < d);
}
DayMacros compute_daily_kcal_and_prot(
  const ProductDB& db,
  const std::string& batches_csv,
  const std::string& extras_csv,
  const Date& start,
  int days
) {
  DayMacros out;
  Date end = add_days(start, days - 1);

  // init 0
  Date cur = start;
  for (int i = 0; i < days; ++i) {
    const auto key = format_date(cur);
    out.kcal[key] = 0.0;
    out.prot[key] = 0.0;
    cur = add_days(cur, 1);
  }

  // batches
  auto blines = read_lines(batches_csv);
  for (size_t i = 1; i < blines.size(); ++i) {
    auto c = split_csv_simple(blines[i]);
    if (c.size() < 6) continue;

    Date bstart{};
    if (!parse_date_yyyy_mm_dd(c[1], bstart)) continue;
    int bdays = std::stoi(c[2]);
    if (bdays <= 0) continue;

    std::string pid = c[3];
    double qty = std::stod(c[4]);

    auto p = db.get_by_id(pid);
    if (!p.has_value()) continue;

    double kcal_day = (qty * p->kcal_per_100 / 100.0) / (double)bdays;
    double prot_day = (qty * p->prot_per_100 / 100.0) / (double)bdays;

    for (int k = 0; k < bdays; ++k) {
      Date d = add_days(bstart, k);
      if (!in_range(d, start, end)) continue;
      const auto key = format_date(d);
      out.kcal[key] += kcal_day;
      out.prot[key] += prot_day;
    }
  }

  auto elines = read_lines(extras_csv);
  for (size_t i = 1; i < elines.size(); ++i) {
    auto c = split_csv_simple(elines[i]);
    if (c.size() < 4) continue;  // date,kcal,prot,comment

    Date d{};
    if (!parse_date_yyyy_mm_dd(c[0], d)) continue;
    if (!in_range(d, start, end)) continue;

    double kcal = std::stod(c[1]);
    double prot = std::stod(c[2]);

    const auto key = format_date(d);
    out.kcal[key] += kcal;
    out.prot[key] += prot;
  }


  return out;
}


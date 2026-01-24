# DailyApp

![Language](https://img.shields.io/badge/language-C%2B%2B17-blue)
![Analytics](https://img.shields.io/badge/analytics-Python-yellow)
![Build](https://img.shields.io/badge/build-CMake-green)
![Status](https://img.shields.io/badge/status-personal%20project-lightgrey)

**DailyApp** is a personal *quantified-self* project designed to track, process, and analyze
everyday metrics such as food intake and body weight.

The project deliberately separates:
- **C++** for data modeling, business logic, and numerical correctness
- **Python** for data analysis and visualization

All data is stored in simple, explicit formats (CSV).  
No proprietary tools, cloud services, or spreadsheets are required.

---

## Project Goals

- Track personal metrics in a transparent and reproducible way
- Keep full control over data formats and computations
- Avoid black-box tools and opaque analytics
- Build a modular foundation for future trackers
  (finance, sport, sleep, etc.)

This project is both a practical tool and a learning exercise in software architecture,
data handling, and numerical reasoning.

---

## Repository Architecture

    DailyApp/
    ├── food-tracker/        # Food intake tracking (C++)
    │   ├── include/         # Domain models & business logic
    │   ├── src/             # Implementations
    │   └── CMakeLists.txt
    │
    ├── mass-tracker/        # Body weight tracking (C++)
    │   ├── include/         # Data structures & storage
    │   ├── src/
    │   ├── analytics/       # Python visualization scripts
    │   └── CMakeLists.txt
    │
    ├── CMakeLists.txt       # Top-level build configuration
    └── README.md

The repository layout is intentional and reflects a clear separation of responsibilities:

- **Trackers** are independent modules
- **C++** handles all data modeling and computations
- **Python** is used strictly for post-processing and visualization

---

## Food Tracker

The **food-tracker** is a command-line tool written in C++ that computes daily nutritional
intake from structured CSV inputs.

### Responsibilities
- Manage a product database (kcal, protein content, units)
- Track food consumption using *batches* spread over multiple days
- Handle punctual extras (meals outside of batch planning)
- Compute daily totals (energy, protein)

### Internal Design

The implementation is split into clear, domain-oriented components:

- **CSV parsing**  
  Robust reading and writing of structured data files
- **Date handling**  
  Explicit date arithmetic and validation
- **Product database**  
  Centralized storage of nutritional information
- **Calculation layer**  
  All nutritional logic (distribution over days, aggregation)

The food-tracker is intentionally **headless**:
it produces reliable data but performs no visualization.

---

## Mass Tracker

The **mass-tracker** is a C++ command-line tool for tracking body weight over time.

### Responsibilities
- Record weight measurements
- Store historical data persistently
- Export data in CSV format
- Enable downstream analysis

### Analytics

The `analytics/` directory contains Python scripts that:
- load exported CSV data
- generate plots and trends
- perform no business logic

This keeps the visualization layer simple and replaceable.

---

## Data & Analytics Philosophy

> **C++ produces the truth. Python visualizes it.**

- All numerical logic lives in C++
- Python scripts only consume already-computed data
- No duplication of business rules across languages

This design minimizes inconsistencies and makes debugging straightforward.

---

## Build System

The project uses **CMake** and requires a modern C++ compiler (C++17 or newer).

- Each tracker can be built independently
- A top-level `CMakeLists.txt` orchestrates the project

---

## Planned Extensions

- Analytics for the food-tracker (daily kcal / protein plots)
- Standardized data directories and formats
- Additional trackers (finance, sport, sleep, etc.)
- Cross-tracker aggregation and summaries

---

## Notes

This repository is a personal project focused on:
- architectural clarity
- explicit data handling
- long-term maintainability

It is not intended to be a polished end-user product, but a controlled and extensible
technical foundation.


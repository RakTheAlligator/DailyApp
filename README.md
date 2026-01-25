# DailyApp

DailyApp is a small C++20 monorepo to track daily metrics (weight, food, etc.) with a single CLI entrypoint.

The goals are:
- one executable: DailyApp
- centralized runtime data in DailyApp/data
- optional Python analytics scripts
- no external C++ dependencies (Linux / Ubuntu friendly)

---

## Repository layout

DailyApp/
CMakeLists.txt  
dailyapp/              root CLI launcher (router)  
weight-tracker/        weight tracker library + CLI  
food-tracker/          food tracker library + CLI  
analytics/             Python scripts for plots  
data/                  runtime CSV / PNG files (ignored by git)

---

## Build (Ubuntu)

From the repository root:

cmake -S . -B build -DCMAKE_BUILD_TYPE=Debug  
cmake --build build -j

The binary is generated here:

build/bin/DailyApp

---

## Usage

Global help:

./build/bin/DailyApp --help

### Weight tracker

./build/bin/DailyApp weight --help  
./build/bin/DailyApp weight add 2026-01-24 62kg  
./build/bin/DailyApp weight history  
./build/bin/DailyApp weight remove 2026-01-24  

### Food tracker

./build/bin/DailyApp food --help  
./build/bin/DailyApp food list  

Draft workflow:

./build/bin/DailyApp food draft-new 2026-01-24 7  
./build/bin/DailyApp food draft-add bread 675g "for the week"  
./build/bin/DailyApp food draft-summary  
./build/bin/DailyApp food draft-commit  

History and plots:

./build/bin/DailyApp food history  

---

## Data files

All runtime files are stored under:

DailyApp/data/

Typical files:
- weight_history.csv
- weight_history.png
- food_products.csv
- food_batches.csv
- food_extras.csv
- food_history.csv
- food_history.png
- draft.csv

Note: the data directory is ignored by git (personal data).

---

## Analytics (Python)

Some commands generate plots using Python scripts:

analytics/weight_history.py  
analytics/food_history.py  

Optional Python environment:

python3 -m venv .venv  
source .venv/bin/activate  
pip install -r analytics/requirements.txt  

Running:

./build/bin/DailyApp weight history  
./build/bin/DailyApp food history  

will also generate PNG plots in DailyApp/data.

---

## Debugging (VS Code)

Only one target needs to be debugged: DailyApp.

The launcher routes all commands to the trackers, so you never need to switch
debug targets for food or weight.

---

## Adding a new tracker

Example: money tracker

1. Create money-tracker/ with:
   - include/MoneyCli.hpp exposing:
     int money::run(std::span<const std::string_view> args);
   - src/MoneyCli.cpp implementing the CLI
   - CMakeLists.txt building money_tracker_lib

2. Link it in dailyapp/CMakeLists.txt:
   target_link_libraries(DailyApp PRIVATE money_tracker_lib)

3. Add routing logic in dailyapp/src/main.cpp.

---

## License

Personal project. Use at your own risk.

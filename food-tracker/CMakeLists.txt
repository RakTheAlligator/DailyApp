cmake_minimum_required(VERSION 3.20)

project(food_tracker
    VERSION 0.1.0
    DESCRIPTION "Daily food tracker (console) with persistent history"
    LANGUAGES CXX
)

# --- C++ standard ---
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# --- executable ---
add_executable(food_tracker
    src/main.cpp
)

# --- include path ---
target_include_directories(food_tracker
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/include
)
# --- warnings (GCC/Clang) ---
if (CMAKE_CXX_COMPILER_ID MATCHES "Clang|GNU")
    target_compile_options(food_tracker PRIVATE -Wall -Wextra -Wpedantic)
endif()

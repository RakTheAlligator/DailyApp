cmake_minimum_required(VERSION 3.20)

project(food_tracker_project LANGUAGES CXX)

add_executable(food_tracker
    src/main.cpp
)

target_compile_features(food_tracker PRIVATE cxx_std_20)

target_include_directories(food_tracker
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/include
)

if (CMAKE_CXX_COMPILER_ID MATCHES "Clang|GNU")
    target_compile_options(food_tracker PRIVATE -Wall -Wextra -Wpedantic)
endif()
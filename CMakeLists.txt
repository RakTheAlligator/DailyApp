cmake_minimum_required(VERSION 3.20)

project(DailyApp LANGUAGES CXX)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

# Build all trackers
add_subdirectory(food-tracker)
add_subdirectory(mass-tracker)

cmake_minimum_required(VERSION 3.20)
project(DailyApp LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

option(DAILYAPP_BUILD_TRACKER_BINS "Build standalone tracker executables" OFF)

add_subdirectory(weight-tracker)
add_subdirectory(food-tracker)
add_subdirectory(dailyapp)

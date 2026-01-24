cmake_minimum_required(VERSION 3.20)

project(weight_tracker_project LANGUAGES CXX)

add_executable(weight_tracker
    src/main.cpp
    src/Storage.cpp
)

target_compile_features(weight_tracker PRIVATE cxx_std_20)

target_compile_definitions(weight_tracker PRIVATE
    WEIGHT_TRACKER_DATA_DIR="${CMAKE_SOURCE_DIR}/data"
    DAILYAPP_ROOT_DIR="${CMAKE_SOURCE_DIR}"
)

target_include_directories(weight_tracker
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/include
)

if (CMAKE_CXX_COMPILER_ID MATCHES "Clang|GNU")
    target_compile_options(weight_tracker PRIVATE -Wall -Wextra -Wpedantic)
endif()

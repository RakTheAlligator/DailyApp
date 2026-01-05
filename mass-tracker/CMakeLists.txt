cmake_minimum_required(VERSION 3.20)

project(mass_tracker_project LANGUAGES CXX)

add_executable(mass_tracker
    src/main.cpp
    src/Storage.cpp
)

target_compile_features(mass_tracker PRIVATE cxx_std_20)

target_include_directories(mass_tracker
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/include
)

if (CMAKE_CXX_COMPILER_ID MATCHES "Clang|GNU")
    target_compile_options(mass_tracker PRIVATE -Wall -Wextra -Wpedantic)
endif()

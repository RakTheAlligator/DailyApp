add_library(food_tracker_lib
    src/FoodCli.cpp
    src/Csv.cpp
    src/Date.cpp
    src/Calculator.cpp
    src/ProductDB.cpp
)

target_include_directories(food_tracker_lib PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_compile_features(food_tracker_lib PUBLIC cxx_std_20)

target_compile_definitions(food_tracker_lib PUBLIC
    DAILYAPP_ROOT_DIR="${CMAKE_SOURCE_DIR}"
    DAILYAPP_DATA_DIR="${CMAKE_SOURCE_DIR}/data"
)

# exe standalone optionnel (si tu gardes un src/main.cpp wrapper)
if(DAILYAPP_BUILD_TRACKER_BINS)
    add_executable(food_tracker src/main.cpp)
    target_link_libraries(food_tracker PRIVATE food_tracker_lib)
endif()

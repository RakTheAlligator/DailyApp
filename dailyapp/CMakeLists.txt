add_executable(DailyApp src/main.cpp)
target_compile_features(DailyApp PRIVATE cxx_std_20)

target_link_libraries(DailyApp PRIVATE weight_tracker_lib food_tracker_lib)

target_compile_definitions(DailyApp PRIVATE
    DAILYAPP_ROOT_DIR="${CMAKE_SOURCE_DIR}"
    DAILYAPP_DATA_DIR="${CMAKE_SOURCE_DIR}/data"
)

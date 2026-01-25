add_library(weight_tracker_lib
    src/Storage.cpp
    src/WeightCli.cpp
)
target_include_directories(weight_tracker_lib PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)
target_compile_features(weight_tracker_lib PUBLIC cxx_std_20)
target_compile_definitions(weight_tracker_lib PUBLIC
    DAILYAPP_ROOT_DIR="${CMAKE_SOURCE_DIR}"
    DAILYAPP_DATA_DIR="${CMAKE_SOURCE_DIR}/data"
)

if(DAILYAPP_BUILD_TRACKER_BINS)
  add_executable(weight_tracker src/main.cpp)
  target_link_libraries(weight_tracker PRIVATE weight_tracker_lib)
  target_compile_definitions(weight_tracker PRIVATE
      DAILYAPP_ROOT_DIR="${CMAKE_SOURCE_DIR}"
      DAILYAPP_DATA_DIR="${CMAKE_SOURCE_DIR}/data"
  )
endif()

find_package(GTest CONFIG REQUIRED)
find_package(nlohmann_json CONFIG REQUIRED)

add_library(stats STATIC
    src/registry.cpp
)

target_include_directories(stats PUBLIC include)
target_link_libraries(stats PRIVATE nlohmann_json::nlohmann_json)

add_executable(stats_tests
    tests/counter_tests.cpp
    tests/histogram_tests.cpp
    tests/running_avg_tests.cpp
    tests/registry_tests.cpp
)

target_link_libraries(stats_tests PRIVATE stats GTest::gtest_main)

include(GoogleTest)
gtest_discover_tests(stats_tests)

find_package(nlohmann_json CONFIG REQUIRED)
find_package(GTest CONFIG REQUIRED)

add_library(isa INTERFACE)

target_include_directories(isa INTERFACE include)
target_link_libraries(isa INTERFACE primitives)

add_executable(isa_tests
    tests/reg_id_tests.cpp
)

target_link_libraries(isa_tests PRIVATE primitives isa nlohmann_json::nlohmann_json GTest::gtest_main)
include(GoogleTest)
gtest_discover_tests(isa_tests)

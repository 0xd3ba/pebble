find_package(GTest CONFIG REQUIRED)
find_package(CLI11 CONFIG REQUIRED)

add_library(utils INTERFACE)
target_include_directories(utils INTERFACE include)

add_executable(utils_tests
    tests/bits_tests.cpp
    tests/argparse_tests.cpp
)

target_link_libraries(utils_tests PRIVATE utils CLI11::CLI11 GTest::gtest_main)
include(GoogleTest)
gtest_discover_tests(utils_tests)

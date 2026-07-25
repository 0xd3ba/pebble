find_package(GTest CONFIG REQUIRED)

add_library(primitives STATIC
    src/register.cpp
)

target_include_directories(primitives PUBLIC include)

add_executable(primitives_tests
    tests/register_tests.cpp
    tests/fifo_tests.cpp
    tests/cam_tests.cpp
)

target_link_libraries(primitives_tests PRIVATE primitives GTest::gtest_main)

include(GoogleTest)
gtest_discover_tests(primitives_tests)

find_package(GTest CONFIG REQUIRED)

add_library(uarch INTERFACE)
target_include_directories(uarch INTERFACE include)

add_executable(uarch_tests
    tests/prf_tests.cpp
)
target_link_libraries(uarch_tests PRIVATE uarch primitives isa utils GTest::gtest_main)

include(GoogleTest)
gtest_discover_tests(uarch_tests)

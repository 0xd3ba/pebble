find_package(nlohmann_json CONFIG REQUIRED)
find_package(spdlog CONFIG REQUIRED)
find_package(GTest CONFIG REQUIRED)

add_library(isa STATIC
    src/decoder.cpp
    src/disassembler.cpp
    src/ops.cpp
    src/functional.cpp
)

target_include_directories(isa PUBLIC include)
target_link_libraries(isa PUBLIC primitives utils PRIVATE spdlog::spdlog)

add_executable(isa_tests
    tests/reg_id_tests.cpp
    tests/decoder_tests.cpp
    tests/arf_tests.cpp
    tests/flat_memory_tests.cpp
    tests/csrf_tests.cpp
    tests/ops_tests.cpp
    tests/functional_tests.cpp
)

target_link_libraries(isa_tests PRIVATE primitives isa nlohmann_json::nlohmann_json spdlog::spdlog GTest::gtest_main)
include(GoogleTest)
gtest_discover_tests(isa_tests)

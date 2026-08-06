find_package(spdlog CONFIG REQUIRED)
find_package(CLI11 CONFIG REQUIRED)

add_executable(functional_cpu src/main.cpp)
target_link_libraries(functional_cpu PRIVATE loader isa spdlog::spdlog CLI11::CLI11)
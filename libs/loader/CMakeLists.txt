find_package(elfio CONFIG REQUIRED)

add_library(loader STATIC src/loader.cpp)
target_include_directories(loader PUBLIC include)
target_link_libraries(loader PRIVATE elfio::elfio)

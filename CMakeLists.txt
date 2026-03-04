cmake_minimum_required(VERSION 3.16)
project(Strata VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Use pkg-config to find ncursesw (wide-char variant required for Unicode drawing)
find_package(PkgConfig REQUIRED)
pkg_check_modules(NCURSESW REQUIRED ncursesw)
pkg_check_modules(PANELW REQUIRED panelw)

file(GLOB_RECURSE STRATA_SOURCES
    "${CMAKE_SOURCE_DIR}/src/**/*.cpp"
    "${CMAKE_SOURCE_DIR}/src/*.cpp"
)

add_library(strata STATIC ${STRATA_SOURCES})
target_include_directories(strata
    PUBLIC  "${CMAKE_SOURCE_DIR}/include"
            ${NCURSESW_INCLUDE_DIRS}
    PRIVATE "${CMAKE_SOURCE_DIR}/src"
)
target_link_libraries(strata PUBLIC ${NCURSESW_LIBRARIES} ${PANELW_LIBRARIES})
target_compile_options(strata PRIVATE -Wall -Wextra -Wpedantic)

add_executable(strata_test test.cpp)
target_link_libraries(strata_test PRIVATE strata)

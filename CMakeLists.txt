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
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
        ${NCURSESW_INCLUDE_DIRS}
    PRIVATE "${CMAKE_SOURCE_DIR}/src"
)
target_link_libraries(strata PUBLIC ${NCURSESW_LIBRARIES} ${PANELW_LIBRARIES})
target_compile_options(strata PRIVATE -Wall -Wextra -Wpedantic)

add_executable(strata_test test.cpp)
target_link_libraries(strata_test PRIVATE strata)

# ── Install ──────────────────────────────────────────────────────────────────

include(GNUInstallDirs)
include(CMakePackageConfigHelpers)

install(TARGETS strata
    EXPORT StrataTargets
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
)

install(DIRECTORY include/Strata
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
)

install(EXPORT StrataTargets
    FILE StrataTargets.cmake
    NAMESPACE Strata::
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/Strata
)

configure_package_config_file(
    "${CMAKE_SOURCE_DIR}/cmake/StrataConfig.cmake.in"
    "${CMAKE_CURRENT_BINARY_DIR}/StrataConfig.cmake"
    INSTALL_DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/Strata
)

write_basic_package_version_file(
    "${CMAKE_CURRENT_BINARY_DIR}/StrataConfigVersion.cmake"
    VERSION ${PROJECT_VERSION}
    COMPATIBILITY SameMajorVersion
)

install(FILES
    "${CMAKE_CURRENT_BINARY_DIR}/StrataConfig.cmake"
    "${CMAKE_CURRENT_BINARY_DIR}/StrataConfigVersion.cmake"
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/Strata
)

# pkg-config file (so users can also use pkg-config --libs strata)
configure_file(
    "${CMAKE_SOURCE_DIR}/cmake/strata.pc.in"
    "${CMAKE_CURRENT_BINARY_DIR}/strata.pc"
    @ONLY
)

install(FILES "${CMAKE_CURRENT_BINARY_DIR}/strata.pc"
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/pkgconfig
)

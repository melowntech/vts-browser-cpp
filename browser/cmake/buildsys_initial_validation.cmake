
# check that build type is set
if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
    message(FATAL_ERROR "CMAKE_BUILD_TYPE is not set.")
endif()

# disable in-source builds
set(CMAKE_DISABLE_IN_SOURCE_BUILD ON)
set(CMAKE_DISABLE_SOURCE_CHANGES ON)
if("${CMAKE_BINARY_DIR}" STREQUAL "${CMAKE_SOURCE_DIR}")
    message(FATAL_ERROR "In-source build is disabled. Remove the already generated files and start again from dedicated build directory.")
endif()

# earlier versions of CMake on apple had an error that include paths were not considered SYSTEM where they should be
if(APPLE)
    cmake_minimum_required(VERSION 3.9.1 FATAL_ERROR)
endif()



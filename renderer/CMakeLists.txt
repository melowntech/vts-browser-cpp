# bootstrap build system
cmake_minimum_required(VERSION 2.8.11)
include(buildsys/cmake/buildsys.cmake)

#global options
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

# project
setup_project(renderer "${BUILDSYS_PACKAGE_VERSION}")

# 3rd party dependencies
find_package(Boost 1.46 REQUIRED COMPONENTS thread program_options filesystem system date_time serialization regex chrono iostreams)
link_directories(${Boost_LIBRARY_DIRS})
include_directories(SYSTEM ${Boost_INCLUDE_DIRS})

find_package(GDAL REQUIRED)
include_directories(${GDAL_INCLUDE_DIR})
find_package(Proj REQUIRED)
include_directories(${PROJ_INCLUDE_DIR})
find_package(GeographicLib REQUIRED)
include_directories(${GEOGRAPHICLIB_INCLUDE_DIR})

find_package(JPEG REQUIRED)
find_package(PNG REQUIRED)
find_package(CURL REQUIRED)
find_package(magic REQUIRED)
find_package(URIPARSER REQUIRED)

find_package(Eigen3 3.2 REQUIRED)
include_directories(${EIGEN3_INCLUDE_DIR})

find_package(LibProc REQUIRED)
include_directories(${LIBPROC_INCLUDE_DIR})

include(glfw.cmake)

# dependencies
add_subdirectory(src/dbglog)
add_subdirectory(src/utility)
add_subdirectory(src/math)
add_subdirectory(src/geometry)
add_subdirectory(src/imgproc)
add_subdirectory(src/jsoncpp)
add_subdirectory(src/service)
add_subdirectory(src/geo)
add_subdirectory(src/half)

# vts libraries
set(BUILDSYS_NOBUILD_TARGET_vts-libs ON)
add_subdirectory(src/vts-libs)

# renderer
add_subdirectory(src/renderer)



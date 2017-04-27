# bootstrap build system
cmake_minimum_required(VERSION 2.8.11)
project(vts-browser-cpp)
# by default, this is OSS build (can be overrided by customer machinery)
set(BUILDSYS_DEFAULT_CUSTOMER_NAME oss)
include(buildsys/cmake/buildsys.cmake)

# temporarily disable warnings-as-errors
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -w")
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -w")

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

find_package(JsonCPP REQUIRED)
include_directories(${JSONCPP_INCLUDE_DIR})

find_package(Eigen3 3.2 REQUIRED)
include_directories(${EIGEN3_INCLUDE_DIR})

find_package(LibProc REQUIRED)
include_directories(${LIBPROC_INCLUDE_DIR})

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
add_subdirectory(src/http)
add_subdirectory(src/glad)
include(src/glfw.cmake)
include(src/lodepng.cmake)
include(src/nuklear.cmake)

# vts libraries
buildsys_nobuild_target(vts-libs)
buildsys_nobuild_target(vts-libs-core:tilestorage)
buildsys_nobuild_target(vts-libs-core:vts0)
add_subdirectory(src/vts-libs)

# renderer
add_subdirectory(src/browser)



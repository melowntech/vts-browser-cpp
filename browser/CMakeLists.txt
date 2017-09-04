# bootstrap build system
cmake_minimum_required(VERSION 2.8.11)
project(vts-browser)

# by default, this is OSS build (can be overrided by customer machinery)
set(BUILDSYS_DEFAULT_CUSTOMER_NAME oss)
include(buildsys/cmake/buildsys.cmake)

enable_hidden_visibility()

# 3rd party dependencies
find_package(JPEG REQUIRED)
find_package(PNG REQUIRED)
find_package(CURL REQUIRED)
find_package(Boost 1.46 REQUIRED
  COMPONENTS thread program_options filesystem system
  date_time serialization regex chrono iostreams)
link_directories(${Boost_LIBRARY_DIRS})
include_directories(SYSTEM ${Boost_INCLUDE_DIRS})
find_package(GDAL 2 REQUIRED)
include_directories(SYSTEM ${GDAL_INCLUDE_DIR})
find_package(Proj REQUIRED)
include_directories(${PROJ_INCLUDE_DIR})
find_package(GeographicLib REQUIRED)
include_directories(${GEOGRAPHICLIB_INCLUDE_DIR})
find_package(JsonCPP REQUIRED)
include_directories(${JSONCPP_INCLUDE_DIR})
find_package(Eigen3 REQUIRED)
include_directories(${EIGEN3_INCLUDE_DIR})
add_subdirectory(src/lodepng)
include_directories(src/lodepng)
add_subdirectory(src/glad)
include_directories(src/glad)
add_subdirectory(src/nuklear)
include_directories(src/nuklear)

# our dependencies
add_subdirectory(src/dbglog)
add_subdirectory(src/utility)
add_subdirectory(src/math)
add_subdirectory(src/geometry)
add_subdirectory(src/imgproc)
add_subdirectory(src/jsoncpp)
add_subdirectory(src/geo)
add_subdirectory(src/half)
add_subdirectory(src/http)
add_subdirectory(src/sdl)

# vts libraries
buildsys_nobuild_target(vts-libs-core)
buildsys_nobuild_target(vts-libs)
add_subdirectory(src/vts-libs)

# browser library
add_subdirectory(src/vts-browser-lib)
include_directories(src/vts-browser-lib/include)

# renderer library
add_subdirectory(src/vts-renderer-lib)
include_directories(src/vts-renderer-lib/include)

if(IOS)

  # ios application
  message(STATUS "including vts-browser-ios")
  add_subdirectory(src/vts-browser-ios)

else()

  # desktop application
  message(STATUS "including vts-browser-desktop")
  add_subdirectory(src/vts-browser-desktop)

endif()




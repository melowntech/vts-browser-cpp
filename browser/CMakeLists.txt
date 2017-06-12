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
find_package(GDAL REQUIRED)
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

# dependencies
add_subdirectory(src/dbglog)
add_subdirectory(src/utility)
add_subdirectory(src/math)
add_subdirectory(src/geometry)
add_subdirectory(src/imgproc)
add_subdirectory(src/jsoncpp)
add_subdirectory(src/geo)
add_subdirectory(src/half)
add_subdirectory(src/http)

# vts libraries
buildsys_nobuild_target(vts-libs-core)
buildsys_nobuild_target(vts-libs)
add_subdirectory(src/vts-libs)

# browser library
add_subdirectory(src/vts-browser-lib)
include_directories(src/vts-browser-lib/include)

# glfw-based example browser
find_package(glfw3)

# build glfw browser only when glfw3 library is found
if(glfw3_FOUND
    AND NOT DEFINED BUILDSYS_NOBUILD_TARGET_vts-browser-glfw)
  message(STATUS "including vts-browser-glfw")
  include_directories(src/glad)
  include_directories(src/nuklear)
  if(DEFINED GLFW3_INCLUDE_DIR)
    include_directories(${GLFW3_INCLUDE_DIR})
  endif()
  add_subdirectory(src/glad)
  add_subdirectory(src/nuklear)
  add_subdirectory(src/vts-browser-glfw)
else()
  message(STATUS "excluding vts-browser-glfw")
endif()

# QT5-based example browser
find_package(Qt5Gui QUIET)

if(Qt5Gui_FOUND
    AND NOT DEFINED BUILDSYS_NOBUILD_TARGET_vts-browser-qt)
  message(STATUS "including vts-browser-qt")
  add_subdirectory(src/vts-browser-qt)
else()
  message(STATUS "excluding vts-browser-qt")
endif()


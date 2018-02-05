# bootstrap build system
cmake_minimum_required(VERSION 2.8.11)
project(vts-browser)

cmake_policy(SET CMP0042 NEW) # use rpath

# by default, this is OSS build (can be overriden by customer machinery)
set(BUILDSYS_DEFAULT_CUSTOMER_NAME oss)
include(buildsys/cmake/buildsys.cmake)

# Bump shared libraries version here
set(vts-browser_SO_VERSION 0.0.0)

enable_hidden_visibility()

# find 3rd party dependencies
message(STATUS "looking for 3rd-party dependencies")
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
add_subdirectory(src/glad)

# vts dependencies
message(STATUS "including vts dependencies")
add_subdirectory(src/dbglog)
add_subdirectory(src/utility)
add_subdirectory(src/math)
add_subdirectory(src/geometry)
add_subdirectory(src/imgproc)
add_subdirectory(src/jsoncpp)
add_subdirectory(src/geo)
add_subdirectory(src/half)

# vts libs
message(STATUS "including vts libs")
buildsys_nobuild_target(vts-libs-core)
buildsys_nobuild_target(vts-libs)
add_subdirectory(src/vts-libs)

# install paths
set(INCLUDE_INSTALL_DIR include)
set(LIB_INSTALL_DIR lib)
set(BIN_INSTALL_DIR bin)
set(SHARE_INSTALL_DIR share)

# cmake functions
include(cmake/pack_data.cmake)

# vts browser
message(STATUS "including vts browser and renderer libraries")
if(IOS)
    set(LIB_TYPE STATIC)
    set(LIB_VIS PUBLIC)
else()
    set(LIB_TYPE SHARED)
    set(LIB_VIS PRIVATE)
endif()
add_subdirectory(src/vts-libbrowser)
include_directories(src/vts-libbrowser/include)
add_subdirectory(src/vts-librenderer)
include_directories(src/vts-librenderer/include)

# application
if(IOS)

    # ios application
    message(STATUS "including vts-browser-ios")
    add_subdirectory(src/vts-browser-ios)

else()

    # find SDL
    cmake_policy(SET CMP0004 OLD) # because SDL installed on some systems has improperly configured libraries
    find_package(SDL2 REQUIRED)
    include_directories(${SDL2_INCLUDE_DIR})

    # minimal application
    message(STATUS "including vts-browser-minimal")
    add_subdirectory(src/vts-browser-minimal)

    # desktop application
    message(STATUS "including vts-browser-desktop")
    add_subdirectory(src/nuklear)
    include_directories(src/nuklear)
    add_subdirectory(src/vts-browser-desktop)

endif()




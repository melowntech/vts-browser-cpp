# bootstrap build system
cmake_minimum_required(VERSION 3.5.1 FATAL_ERROR)
project(vts-browser)

if (APPLE)
    # earlier versions of CMake on apple had an error that include paths were not considered SYSTEM where they should be
    cmake_minimum_required(VERSION 3.9.1 FATAL_ERROR)
endif()

cmake_policy(SET CMP0042 NEW) # use rpath

# by default, this is OSS build (can be overriden by customer machinery)
set(BUILDSYS_DEFAULT_CUSTOMER_NAME oss)
include(buildsys/cmake/buildsys.cmake)

# Bump shared libraries version here
set(vts-browser_SO_VERSION 0.0.0)

# cmake functions
include(cmake/buildsys_ide_groups.cmake)
include(cmake/buildsys_pack_data.cmake)

# compiler options
enable_hidden_visibility()
if(WIN32)
    # disable warning: class 'type' needs to have dll-interface to be used by clients of class 'type2'
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} /wd4251 /wd4275")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /wd4251 /wd4275")
    # enable multi process compilation
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /MP")
    # disable iterator range checks for all builds to allow compatibility with release libraries
    #set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /D_ITERATOR_DEBUG_LEVEL=0")
endif()

# find 3rd party dependencies
message(STATUS "looking for 3rd-party dependencies")
find_package(Boost 1.46 REQUIRED
    COMPONENTS thread program_options filesystem system
    date_time serialization regex chrono iostreams)
link_directories(${Boost_LIBRARY_DIRS})
include_directories(SYSTEM ${Boost_INCLUDE_DIRS})
if(WIN32)
    add_definitions(-DBOOST_ALL_NO_LIB -DBOOST_ALL_DYN_LINK)
    add_definitions(-DBOOST_CONFIG_SUPPRESS_OUTDATED_MESSAGE)
endif()
find_package(GDAL 2 REQUIRED)
include_directories(SYSTEM ${GDAL_INCLUDE_DIR})
find_package(Proj REQUIRED)
include_directories(SYSTEM ${PROJ_INCLUDE_DIR})
find_package(GeographicLib REQUIRED)
include_directories(SYSTEM ${GEOGRAPHICLIB_INCLUDE_DIR})
find_package(JsonCPP REQUIRED)
include_directories(SYSTEM ${JSONCPP_INCLUDE_DIR})
find_package(Eigen3 REQUIRED)
include_directories(SYSTEM ${EIGEN3_INCLUDE_DIR})

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
buildsys_ide_groups(dbglog deps)
buildsys_ide_groups(utility deps)
buildsys_ide_groups(math deps)
buildsys_ide_groups(geometry deps)
buildsys_ide_groups(imgproc deps)
buildsys_ide_groups(jsoncpp deps)
buildsys_ide_groups(geo deps)
buildsys_ide_groups(half deps)

# vts libs
message(STATUS "including vts libs")
buildsys_nobuild_target(vts-libs-core)
buildsys_nobuild_target(vts-libs)
add_subdirectory(src/vts-libs)
buildsys_ide_groups(vts-libs-nucleus deps)

# install paths
set(INCLUDE_INSTALL_DIR include)
set(LIB_INSTALL_DIR lib)
set(BIN_INSTALL_DIR bin)
set(SHARE_INSTALL_DIR share)

# vts browser
message(STATUS "including vts browser and renderer libraries")
if(BUILDSYS_EMBEDDED)
    set(LIB_TYPE STATIC)
    set(LIB_VIS PUBLIC)
else()
    set(LIB_TYPE SHARED)
    set(LIB_VIS PRIVATE)

    # find http and curl
    find_package(CURL REQUIRED)
    include_directories(SYSTEM ${CURL_INCLUDE_DIR})
    add_subdirectory(src/http)
    buildsys_ide_groups(http deps)
endif()
add_subdirectory(src/vts-libbrowser)
include_directories(src/vts-libbrowser/include)
add_subdirectory(src/glad)
add_subdirectory(src/vts-librenderer)
include_directories(src/vts-librenderer/include)

# application
if(BUILDSYS_IOS)
    # ios application
    message(STATUS "including vts-browser-ios")
    add_subdirectory(src/vts-browser-ios)
else()
    # find SDL
    cmake_policy(SET CMP0004 OLD) # because SDL installed on some systems has improperly configured libraries
    find_package(SDL2 REQUIRED)
    include_directories(SYSTEM ${SDL2_INCLUDE_DIR})

    # minimal application
    message(STATUS "including vts-browser-minimal")
    add_subdirectory(src/vts-browser-minimal-c)
    add_subdirectory(src/vts-browser-minimal-cpp)

    # desktop application
    message(STATUS "including vts-browser-desktop")
    add_subdirectory(src/nuklear)
    add_subdirectory(src/vts-browser-desktop)
endif()




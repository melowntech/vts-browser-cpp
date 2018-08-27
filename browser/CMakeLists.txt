# bootstrap build system
cmake_minimum_required(VERSION 3.5.1 FATAL_ERROR)
project(vts-browser)

if(APPLE)
    # earlier versions of CMake on apple had an error that include paths were not considered SYSTEM where they should be
    cmake_minimum_required(VERSION 3.9.1 FATAL_ERROR)
endif()

# disable 32 bit builds
if(NOT CMAKE_SIZEOF_VOID_P EQUAL 8)
    message(FATAL_ERROR "64 bit generator is required.")
endif()

# by default, this is OSS build (can be overriden by customer machinery)
set(BUILDSYS_DEFAULT_CUSTOMER_NAME oss)
include(buildsys/cmake/buildsys.cmake)
buildsys_fix_symlinks(${CMAKE_CURRENT_SOURCE_DIR})

# bump shared libraries version here
set(vts-browser_SO_VERSION 0.0.0)

# include additional buildsys functions
include(cmake/buildsys_ide_groups.cmake)
include(cmake/buildsys_pack_data.cmake)

# compiler options
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

# libraries type
set(VTS_BROWSER_TYPE SHARED CACHE STRING "Type of browser libraries")
set_property(CACHE VTS_BROWSER_TYPE PROPERTY STRINGS STATIC SHARED MODULE FRAMEWORK)
set(VTS_BROWSER_BUILD_LIBRARY ${VTS_BROWSER_TYPE})
if(VTS_BROWSER_TYPE STREQUAL STATIC)
    set(VTS_BROWSER_BUILD_VISIBILITY PUBLIC)
    set(VTS_BROWSER_BUILD_MACRO STATIC)
else()
    set(VTS_BROWSER_BUILD_VISIBILITY PRIVATE)
    set(VTS_BROWSER_BUILD_MACRO SHARED)
endif()
if(VTS_BROWSER_TYPE STREQUAL FRAMEWORK)
    set(VTS_BROWSER_BUILD_LIBRARY SHARED)
endif()

# vts browser library
message(STATUS "including vts browser library")
add_subdirectory(src/vts-libbrowser)
include_directories(src/vts-libbrowser/include)

if(NOT VTS_LIBS_TYPE STREQUAL MODULE)
    # vts renderer library
    message(STATUS "including vts renderer library")
    add_subdirectory(src/glad)
    add_subdirectory(src/vts-librenderer)
    include_directories(src/vts-librenderer/include)

    # application
    if(BUILDSYS_IOS)
        # ios application
        message(STATUS "including vts-browser-ios")
        add_subdirectory(src/vts-browser-ios)
        set(VS_STARTUP_PROJECT vts-browser-ios)
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
        set(VS_STARTUP_PROJECT vts-browser-desktop)
    endif()

    # vts csharp libraries
    if(MSVC)
        message(STATUS "including csharp libraries")
        set(CMAKE_CSharp_FLAGS "${CMAKE_CSharp_FLAGS} -langversion:6 -unsafe")
        add_subdirectory(src/vts-libbrowser-cs)
        add_subdirectory(src/vts-librenderer-cs)
        add_subdirectory(src/vts-browser-minimal-cs)
    endif()
endif()


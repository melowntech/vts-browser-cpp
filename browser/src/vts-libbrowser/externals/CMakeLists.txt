
# disable warnings
if(MSVC)
    string(REPLACE "/W3" "/W0" CMAKE_C_FLAGS ${CMAKE_C_FLAGS})
    string(REPLACE "/W3" "/W0" CMAKE_CXX_FLAGS ${CMAKE_CXX_FLAGS})
endif()
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -w")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -w")

#disable warnings that some files produced no symbols
if(MSVC)
    set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} -ignore:4221")
    set(CMAKE_STATIC_LINKER_FLAGS "${CMAKE_STATIC_LINKER_FLAGS} -ignore:4221")
endif()

buildsys_fix_symlinks(${CMAKE_CURRENT_SOURCE_DIR})

find_package(Proj REQUIRED)
include_directories(SYSTEM ${PROJ_INCLUDE_DIR})
find_package(GeographicLib REQUIRED)
include_directories(SYSTEM ${GEOGRAPHICLIB_INCLUDE_DIR})
find_package(JsonCPP REQUIRED)
include_directories(SYSTEM ${JSONCPP_INCLUDE_DIR})

# vts dependencies
message(STATUS "including vts dependencies")
add_subdirectory(dbglog)
add_subdirectory(utility)
add_subdirectory(jsoncpp)
add_subdirectory(math)
add_subdirectory(half)
# we need just a few files from the following libraries
# it is simpler to just write them directly here rather then modifying their cmake files
add_library(geometry STATIC geometry/parse-obj.cpp)
target_include_directories(geometry PUBLIC geometry)
target_link_libraries(geometry PUBLIC math utility dbglog)
add_library(imgproc STATIC imgproc/rastermask/bitfield.cpp imgproc/rastermask/quadtree.cpp)
target_include_directories(imgproc PUBLIC imgproc)
target_link_libraries(imgproc PUBLIC math utility dbglog)
add_subdirectory(geo)
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
set(VTSLIBS_BROWSER_ONLY TRUE)
add_subdirectory(vts-libs)
buildsys_ide_groups(vts-libs-core deps)

# additional fixes
target_include_directories(jsoncpp SYSTEM INTERFACE ${JSONCPP_INCLUDE_DIR})
target_include_directories(geo SYSTEM INTERFACE ${GDAL_INCLUDE_DIR})


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
add_subdirectory(math)
add_subdirectory(geometry)
add_subdirectory(imgproc)
add_subdirectory(jsoncpp)
add_subdirectory(geo)
add_subdirectory(half)
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
add_subdirectory(vts-libs)
buildsys_ide_groups(vts-libs-nucleus deps)

# additional fixes
target_include_directories(jsoncpp SYSTEM INTERFACE ${JSONCPP_INCLUDE_DIR})
target_include_directories(geo SYSTEM INTERFACE ${GDAL_INCLUDE_DIR})

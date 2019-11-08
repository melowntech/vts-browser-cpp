#
# Set -DEMSCRIPTEN_PREFIX=<path> to the directory with emcc.
#
# Originally taken from:
# https://github.com/mosra/toolchains/blob/master/generic/Emscripten-wasm.cmake
# and modified
#

message(STATUS "****************************")
message(STATUS "*** Using WASM toolchain ***")
message(STATUS "****************************")

set(CMAKE_CROSSCOMPILING TRUE)
set(BUILDSYS_WASM TRUE)
set(BUILDSYS_EMBEDDED TRUE)

if(NOT EMSCRIPTEN_PREFIX)
    if(DEFINED ENV{EMSCRIPTEN})
        file(TO_CMAKE_PATH "$ENV{EMSCRIPTEN}" EMSCRIPTEN_PREFIX)
    else()
        find_file(_EMSCRIPTEN_EMCC_EXECUTABLE emcc)
        if(EXISTS ${_EMSCRIPTEN_EMCC_EXECUTABLE})
            get_filename_component(EMSCRIPTEN_PREFIX ${_EMSCRIPTEN_EMCC_EXECUTABLE} DIRECTORY)
        else()
            set(EMSCRIPTEN_PREFIX "/usr/lib/emscripten")
        endif()
    endif()
endif()
message(STATUS "EMSCRIPTEN_PREFIX: ${EMSCRIPTEN_PREFIX}")

set(CMAKE_C_COMPILER "${EMSCRIPTEN_PREFIX}/emcc")
set(CMAKE_CXX_COMPILER "${EMSCRIPTEN_PREFIX}/em++")
set(CMAKE_AR "${EMSCRIPTEN_PREFIX}/emar" CACHE FILEPATH "Emscripten ar")
set(CMAKE_RANLIB "${EMSCRIPTEN_PREFIX}/emranlib" CACHE FILEPATH "Emscripten ranlib")

set(EMSCRIPTEN_TOOLCHAIN_PATH "${EMSCRIPTEN_PREFIX}/system")
set(CMAKE_FIND_ROOT_PATH ${CMAKE_FIND_ROOT_PATH}
    "${EMSCRIPTEN_TOOLCHAIN_PATH}"
    "${EMSCRIPTEN_PREFIX}")

# what system to look on
set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_PACKAGE ONLY)


set(common_flags "-s WASM=1 -s USE_ZLIB=1 -s USE_LIBPNG=1 -s USE_LIBJPEG=1 -s USE_FREETYPE=1 -s USE_HARFBUZZ=1")
set(CMAKE_C_FLAGS_INIT "${common_flags}")
set(CMAKE_CXX_FLAGS_INIT "${common_flags}")
#set(CMAKE_EXE_LINKER_FLAGS_INIT "${common_flags}")
#set(CMAKE_SHARED_LINKER_FLAGS_INIT "${common_flags}")
#set(CMAKE_STATIC_LINKER_FLAGS_INIT "${common_flags}")
#set(CMAKE_MODULE_LINKER_FLAGS_INIT "${common_flags}")
#set(CMAKE_CXX_FLAGS_RELEASE_INIT "-DNDEBUG -O3")
#set(CMAKE_EXE_LINKER_FLAGS_RELEASE_INIT "-O3 --llvm-lto 1")







set(VTS_BROWSER_TYPE STATIC CACHE STRING "Type of browser libraries" FORCE)



## Prefixes/suffixes for building
#set(CMAKE_STATIC_LIBRARY_PREFIX "")
#set(CMAKE_STATIC_LIBRARY_SUFFIX ".bc")
#set(CMAKE_EXECUTABLE_SUFFIX ".js")
#
## Prefixes/suffixes for finding libraries
#set(CMAKE_FIND_LIBRARY_PREFIXES "")
#set(CMAKE_FIND_LIBRARY_SUFFIXES ".bc")
#
#set(CMAKE_BUILD_WITH_INSTALL_RPATH TRUE)
#set(CMAKE_INCLUDE_SYSTEM_FLAG_CXX "-isystem")



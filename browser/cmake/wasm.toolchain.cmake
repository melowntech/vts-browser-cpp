
message(STATUS "****************************")
message(STATUS "*** Using WASM toolchain ***")
message(STATUS "****************************")

set(BUILDSYS_WASM TRUE)
set(BUILDSYS_EMBEDDED TRUE)
set(CMAKE_CROSSCOMPILING TRUE)
set(CMAKE_SYSTEM_PROCESSOR x86)

# path to the compiler (directory)
if(NOT EMSCRIPTEN_ROOT)
    if(DEFINED ENV{EMSCRIPTEN_ROOT})
        set(EMSCRIPTEN_ROOT "$ENV{EMSCRIPTEN_ROOT}")
    else()
        set(active "$ENV{HOME}/.emscripten")
        if(EXISTS ${active})
            file(STRINGS ${active} lines)
            foreach(line ${lines})
                if(line MATCHES "EMSCRIPTEN_ROOT.*'(.*)'")
                    set(EMSCRIPTEN_ROOT ${CMAKE_MATCH_1})
                endif()
            endforeach()
        else()
            find_file(_EMSCRIPTEN_EMCC_EXECUTABLE emcc)
            if(EXISTS ${_EMSCRIPTEN_EMCC_EXECUTABLE})
                get_filename_component(EMSCRIPTEN_ROOT ${_EMSCRIPTEN_EMCC_EXECUTABLE} DIRECTORY)
            endif()
        endif()
    endif()
endif()
message(STATUS "EMSCRIPTEN_ROOT: ${EMSCRIPTEN_ROOT}")

# build tools
set(CMAKE_C_COMPILER "${EMSCRIPTEN_ROOT}/emcc")
set(CMAKE_CXX_COMPILER "${EMSCRIPTEN_ROOT}/em++")
set(CMAKE_AR "${EMSCRIPTEN_ROOT}/emar" CACHE FILEPATH "Emscripten ar")
set(CMAKE_RANLIB "${EMSCRIPTEN_ROOT}/emranlib" CACHE FILEPATH "Emscripten ranlib")

set(EMSCRIPTEN_TOOLCHAIN_PATH "${EMSCRIPTEN_ROOT}/system")
set(CMAKE_FIND_ROOT_PATH ${CMAKE_FIND_ROOT_PATH}
    "${EMSCRIPTEN_TOOLCHAIN_PATH}"
    "${EMSCRIPTEN_ROOT}")

# what system to look on
set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_PACKAGE ONLY)

# In order for check_function_exists() detection to work, we must signal it to pass an additional flag, which causes the compilation
# to abort if linking results in any undefined symbols. The CMake detection mechanism depends on the undefined symbol error to be raised.
set(CMAKE_REQUIRED_FLAGS "-s ERROR_ON_UNDEFINED_SYMBOLS=1")

set(CMAKE_SYSTEM_INCLUDE_PATH "${EMSCRIPTEN_ROOT_PATH}/system/include")

# -s FETCH_DEBUG=1
# -fsanitize=address
# -s ERROR_ON_UNDEFINED_SYMBOLS=0 # this flag is experimental and may cause issues :(
# -msimd128 # chrome fails saying that it does not understand a type
set(common_flags "-s WASM=1 -s USE_PTHREADS=1 -s FETCH=1 -s USE_ZLIB=1 -s USE_LIBPNG=1 -s USE_LIBJPEG=1 -s USE_FREETYPE=1 -s USE_HARFBUZZ=1 -s USE_WEBGL2=1 -s MIN_WEBGL_VERSION=2 -s MAX_WEBGL_VERSION=2 -s FULL_ES3=1 -s GL_POOL_TEMP_BUFFERS=0 -s DISABLE_EXCEPTION_CATCHING=0 -s FILESYSTEM=0 -s ALLOW_BLOCKING_ON_MAIN_THREAD=0 -s EVAL_CTORS=1 -s STRICT=1 -s STRICT_JS=1")
set(debug_flags "-s ASSERTIONS=2 -s SAFE_HEAP=1 -s STACK_OVERFLOW_CHECK=2 -s DEMANGLE_SUPPORT=1 -s GL_DEBUG=1 -s GL_ASSERTIONS=1 -s PTHREADS_DEBUG=1 -g4 -O0")
set(release_flags "-s GL_TRACK_ERRORS=0 -O3 -DNDEBUG")
set(CMAKE_C_FLAGS_INIT "")
set(CMAKE_CXX_FLAGS_INIT "")
foreach(conf IN ITEMS ${CMAKE_CONFIGURATION_TYPES} ${CMAKE_BUILD_TYPE})
    string(TOUPPER ${conf} conf_upper)
    set(CMAKE_C_FLAGS_${conf_upper}_INIT "${common_flags}")
    set(CMAKE_CXX_FLAGS_${conf_upper}_INIT "${common_flags}")
    if(${conf_upper} MATCHES "DEBUG")
        string(APPEND CMAKE_C_FLAGS_${conf_upper}_INIT " ${debug_flags}")
        string(APPEND CMAKE_CXX_FLAGS_${conf_upper}_INIT " ${debug_flags}")
    else()
        string(APPEND CMAKE_C_FLAGS_${conf_upper}_INIT " ${release_flags}")
        string(APPEND CMAKE_CXX_FLAGS_${conf_upper}_INIT " ${release_flags}")
    endif()
endforeach(conf)




set(VTS_BROWSER_TYPE STATIC CACHE STRING "Type of browser libraries" FORCE)



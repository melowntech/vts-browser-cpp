
message(STATUS "****************************")
message(STATUS "*** Using WASM toolchain ***")
message(STATUS "****************************")

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

# include emscripten toolchain
include(${EMSCRIPTEN_ROOT}/cmake/Modules/Platform/Emscripten.cmake)
if(NOT CMAKE_SYSTEM_NAME)
    message(FATAL_ERROR "CMAKE_SYSTEM_NAME NOT SET IN THE EMSCRIPTEN TOOLCHAIN")
endif()

# configure compile and link flags
# -s FETCH_DEBUG=1
# -fsanitize=address
# -msimd128 # chrome fails saying that it does not understand a type
set(common_flags "-s WASM=1 -s USE_PTHREADS=1 -s FILESYSTEM=1 -s DISABLE_EXCEPTION_CATCHING=0 -s ERROR_ON_UNDEFINED_SYMBOLS=1 -s STRICT=1 -s STRICT_JS=1 -s EVAL_CTORS=1")
set(debug_flags "-s ASSERTIONS=2 -s SAFE_HEAP=1 -s STACK_OVERFLOW_CHECK=2 -s DEMANGLE_SUPPORT=1 -s GL_DEBUG=1 -s GL_ASSERTIONS=1 -s PTHREADS_DEBUG=1 -g4 -O0")
set(release_flags "-s GL_TRACK_ERRORS=0 -O3 -DNDEBUG")
set(CMAKE_C_FLAGS_INIT "${common_flags}" CACHE STRING "")
set(CMAKE_CXX_FLAGS_INIT "${common_flags}" CACHE STRING "")
foreach(conf IN ITEMS ${CMAKE_CONFIGURATION_TYPES} ${CMAKE_BUILD_TYPE})
    string(TOUPPER ${conf} conf_upper)
    if(${conf_upper} MATCHES "DEBUG")
        set(CMAKE_C_FLAGS_${conf_upper}_INIT "${debug_flags}" CACHE STRING "")
        set(CMAKE_CXX_FLAGS_${conf_upper}_INIT "${debug_flags}" CACHE STRING "")
    else()
        set(CMAKE_C_FLAGS_${conf_upper}_INIT "${release_flags}" CACHE STRING "")
        set(CMAKE_CXX_FLAGS_${conf_upper}_INIT "${release_flags}" CACHE STRING "")
    endif()
endforeach(conf)

# configure buildsys
set(BUILDSYS_WASM TRUE)
set(BUILDSYS_EMBEDDED TRUE)

# configure vts
set(VTS_BROWSER_TYPE STATIC CACHE STRING "Type of browser libraries" FORCE)

# fix some detection issues
set(LCONV_SIZE_run_result 0 CACHE STRING "")
set(LCONV_SIZE_run_result__TRYRUN_OUTPUT 0 CACHE STRING "")
set(LONG_DOUBLE_run_result 0 CACHE STRING "")
set(LONG_DOUBLE_run_result__TRYRUN_OUTPUT 0 CACHE STRING "")

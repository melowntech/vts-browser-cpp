
# the SDL app fails to link with latest emscripten with
# undefined symbol: emscripten_sleep
# temporary workaround: ERROR_ON_UNDEFINED_SYMBOLS

define_module(BINARY vts-browser-wasm-sdl DEPENDS
    vts-browser vts-renderer)

set(SRC_LIST
    main.cpp
)

# the current emscripten FETCH works with synchronous operations only and we workaround that with many threads
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -s USE_SDL=2 -s ERROR_ON_UNDEFINED_SYMBOLS=0 --emrun -s ALLOW_MEMORY_GROWTH=1 -s INITIAL_MEMORY=512MB -s MAXIMUM_MEMORY=2GB")

set(CMAKE_EXECUTABLE_SUFFIX ".html")
add_executable(vts-browser-wasm-sdl ${SRC_LIST})
target_link_libraries(vts-browser-wasm-sdl ${MODULE_LIBRARIES})
target_compile_definitions(vts-browser-wasm-sdl PRIVATE ${MODULE_DEFINITIONS})
buildsys_binary(vts-browser-wasm-sdl)
buildsys_ide_groups(vts-browser-wasm-sdl apps)

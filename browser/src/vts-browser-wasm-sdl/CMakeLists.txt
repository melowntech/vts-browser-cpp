
define_module(BINARY vts-browser-wasm-sdl DEPENDS
    vts-browser vts-renderer)

set(SRC_LIST
    main.cpp
)

# the "-Wl,--no-check-features is temporary workaround until https://github.com/emscripten-ports/HarfBuzz/issues/1 is fixed"
# the current emscripten FETCH works with synchronous operations only and we workaround that with many threads
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -s USE_SDL=2 -Wl,--no-check-features --emrun -s PTHREAD_POOL_SIZE=30 -s ALLOW_MEMORY_GROWTH=1 -s TOTAL_MEMORY=134217728 -s WASM_MEM_MAX=1073741824")

add_executable(vts-browser-wasm-sdl ${SRC_LIST})
target_link_libraries(vts-browser-wasm-sdl ${MODULE_LIBRARIES})
target_compile_definitions(vts-browser-wasm-sdl PRIVATE ${MODULE_DEFINITIONS})
buildsys_binary(vts-browser-wasm-sdl)
buildsys_ide_groups(vts-browser-wasm-sdl apps)


foreach(conf IN ITEMS ${CMAKE_CONFIGURATION_TYPES} ${CMAKE_BUILD_TYPE})
    string(TOUPPER ${conf} conf_upper)
    set_target_properties(vts-browser-wasm-sdl PROPERTIES ${conf_upper}_POSTFIX .html)
endforeach(conf)

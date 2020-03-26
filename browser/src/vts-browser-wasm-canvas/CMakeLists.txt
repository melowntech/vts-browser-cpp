
define_module(BINARY vts-browser-wasm-canvas DEPENDS
    vts-browser vts-renderer jsoncpp)

set(SRC_LIST
    common.hpp
    inputs.cpp
    jsInterop.cpp
    main.cpp
    render.cpp
    render.hpp
    timer.hpp
    toHtml.cpp
)

set(WEBFILES_LIST
    index.html
    vts.js
    vts.css
)

# the current emscripten FETCH works with synchronous operations only and we workaround that with many threads
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -s OFFSCREENCANVAS_SUPPORT=1 -s PROXY_TO_PTHREAD=1 -s OFFSCREENCANVASES_TO_PTHREAD='#display' -s PTHREAD_POOL_SIZE=30 -s ALLOW_MEMORY_GROWTH=0 -s INITIAL_MEMORY=512MB -s MAXIMUM_MEMORY=1024MB -s EXTRA_EXPORTED_RUNTIME_METHODS='[cwrap,printErr]' -s ERROR_ON_UNDEFINED_SYMBOLS=0")

set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} --source-map-base http://localhost:6931")

add_executable(vts-browser-wasm-canvas ${SRC_LIST} ${WEBFILES_LIST})
target_link_libraries(vts-browser-wasm-canvas ${MODULE_LIBRARIES} "GL")
target_compile_definitions(vts-browser-wasm-canvas PRIVATE ${MODULE_DEFINITIONS})
buildsys_binary(vts-browser-wasm-canvas)
buildsys_ide_groups(vts-browser-wasm-canvas apps)

foreach(conf IN ITEMS ${CMAKE_CONFIGURATION_TYPES} ${CMAKE_BUILD_TYPE})
    string(TOUPPER ${conf} conf_upper)
    set_target_properties(vts-browser-wasm-canvas PROPERTIES ${conf_upper}_POSTFIX .js)
    foreach(file IN ITEMS ${WEBFILES_LIST})
        configure_file("${file}" "${CMAKE_RUNTIME_OUTPUT_DIRECTORY_${conf_upper}}/${file}" COPYONLY)
    endforeach()
endforeach(conf)


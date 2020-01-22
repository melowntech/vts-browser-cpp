
define_module(BINARY vts-browser-wasm-canvas DEPENDS
    vts-browser vts-renderer jsoncpp)

set(SRC_LIST
    common.hpp
    inputs.cpp
    jsInterop.cpp
    main.cpp
    timer.hpp
    toHtml.cpp
)

set(WEBFILES_LIST
    index.html
    vts.js
    vts.css
)

# the current emscripten FETCH works with synchronous operations only and we workaround that with many threads
# google chrome has an issue with growing memory, so we use large initial buffer to avoid the growth at all
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -s PTHREAD_POOL_SIZE=30 -s TOTAL_MEMORY=512MB -s ALLOW_MEMORY_GROWTH=1 -s WASM_MEM_MAX=1024MB -s EXTRA_EXPORTED_RUNTIME_METHODS='[cwrap]'")

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


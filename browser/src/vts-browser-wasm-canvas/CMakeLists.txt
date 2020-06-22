
define_module(BINARY vts-browser-wasm-canvas DEPENDS
    vts-browser vts-renderer)

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

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -s ALLOW_BLOCKING_ON_MAIN_THREAD=0 -s OFFSCREENCANVAS_SUPPORT=1 -s PROXY_TO_PTHREAD=1 -s OFFSCREENCANVASES_TO_PTHREAD='#display' -s ALLOW_MEMORY_GROWTH=1 -s INITIAL_MEMORY=512MB -s MAXIMUM_MEMORY=2GB -s EXTRA_EXPORTED_RUNTIME_METHODS='[cwrap,printErr]'")
#set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} --source-map-base http://localhost:6931")

add_executable(vts-browser-wasm-canvas ${SRC_LIST} ${WEBFILES_LIST})
target_link_libraries(vts-browser-wasm-canvas ${MODULE_LIBRARIES})
target_compile_definitions(vts-browser-wasm-canvas PRIVATE ${MODULE_DEFINITIONS})
buildsys_binary(vts-browser-wasm-canvas)
buildsys_ide_groups(vts-browser-wasm-canvas apps)

foreach(conf IN ITEMS ${CMAKE_CONFIGURATION_TYPES} ${CMAKE_BUILD_TYPE})
    string(TOUPPER ${conf} conf_upper)
    foreach(file IN ITEMS ${WEBFILES_LIST})
        configure_file("${file}" "${CMAKE_RUNTIME_OUTPUT_DIRECTORY_${conf_upper}}/${file}" COPYONLY)
    endforeach()
endforeach(conf)




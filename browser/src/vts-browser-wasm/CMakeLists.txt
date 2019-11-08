
define_module(BINARY vts-browser-wasm DEPENDS
    vts-browser vts-renderer)

set(SRC_LIST
    main.cpp
)

add_executable(vts-browser-wasm ${SRC_LIST})
target_link_libraries(vts-browser-wasm ${MODULE_LIBRARIES})
target_compile_definitions(vts-browser-wasm PRIVATE ${MODULE_DEFINITIONS})
target_compile_options(vts-browser-wasm PRIVATE -s USE_SDL=2)
buildsys_binary(vts-browser-wasm)
buildsys_ide_groups(vts-browser-wasm apps)


foreach(conf IN ITEMS ${CMAKE_CONFIGURATION_TYPES} ${CMAKE_BUILD_TYPE})
    string(TOUPPER ${conf} conf_upper)
    set_target_properties(vts-browser-wasm PROPERTIES ${conf_upper}_POSTFIX .html)
endforeach(conf)

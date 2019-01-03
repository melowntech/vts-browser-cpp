
define_module(BINARY vts-browser-two-cameras DEPENDS
    vts-browser vts-renderer SDL2)

set(SRC_LIST
    main.cpp
)

add_executable(vts-browser-two-cameras ${SRC_LIST})
target_link_libraries(vts-browser-two-cameras ${MODULE_LIBRARIES})
target_compile_definitions(vts-browser-two-cameras PRIVATE ${MODULE_DEFINITIONS})
buildsys_binary(vts-browser-two-cameras)
buildsys_ide_groups(vts-browser-two-cameras apps)


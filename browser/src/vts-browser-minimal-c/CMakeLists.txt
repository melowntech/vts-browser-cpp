
define_module(BINARY vts-browser-minimal-c DEPENDS
    vts-browser vts-renderer SDL2)

set(SRC_LIST
    main.c
)

add_executable(vts-browser-minimal-c ${SRC_LIST})
target_link_libraries(vts-browser-minimal-c ${MODULE_LIBRARIES})
target_compile_definitions(vts-browser-minimal-c PRIVATE ${MODULE_DEFINITIONS})
buildsys_binary(vts-browser-minimal-c)
buildsys_ide_groups(vts-browser-minimal-c apps)


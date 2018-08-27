
define_module(BINARY vts-browser-minimal-cpp DEPENDS
    vts-browser vts-renderer SDL2)

set(SRC_LIST
    main.cpp
)

add_executable(vts-browser-minimal-cpp ${SRC_LIST})
target_link_libraries(vts-browser-minimal-cpp ${MODULE_LIBRARIES})
target_compile_definitions(vts-browser-minimal-cpp PRIVATE ${MODULE_DEFINITIONS})
buildsys_binary(vts-browser-minimal-cpp)
buildsys_ide_groups(vts-browser-minimal-cpp apps)


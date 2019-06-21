
define_module(BINARY test-smoothness DEPENDS
    vts-browser vts-renderer SDL2 THREADS)

set(SRC_LIST
    main.cpp
)

add_executable(test-smoothness ${SRC_LIST})
target_link_libraries(test-smoothness ${MODULE_LIBRARIES})
target_compile_definitions(test-smoothness PRIVATE ${MODULE_DEFINITIONS})
buildsys_binary(test-smoothness)
buildsys_ide_groups(test-smoothness apps)


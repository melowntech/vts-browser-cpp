
define_module(BINARY test-two-cameras DEPENDS
    vts-browser vts-renderer SDL2 THREADS)

set(SRC_LIST
    main.cpp
)

add_executable(test-two-cameras ${SRC_LIST})
target_link_libraries(test-two-cameras ${MODULE_LIBRARIES})
target_compile_definitions(test-two-cameras PRIVATE ${MODULE_DEFINITIONS})
buildsys_binary(test-two-cameras)
buildsys_ide_groups(test-two-cameras apps)


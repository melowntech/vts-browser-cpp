
define_module(BINARY vts-extractor DEPENDS vts-browser)

set(SRC_LIST
    main.cpp
)

add_executable(vts-extractor ${SRC_LIST})
target_link_libraries(vts-extractor PRIVATE ${MODULE_LIBRARIES})
target_compile_definitions(vts-extractor PRIVATE ${MODULE_DEFINITIONS})
buildsys_binary(vts-extractor)
buildsys_ide_groups(vts-extractor apps)

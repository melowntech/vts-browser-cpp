define_module(BINARY ${PROJECT_NAME}-core DEPENDS vts-libs-core)

aux_source_directory(. SRC_LIST)
add_library(${PROJECT_NAME}-core SHARED ${SRC_LIST})
target_include_directories(${PROJECT_NAME}-core PRIVATE ../include)
target_link_libraries(${PROJECT_NAME}-core ${MODULE_LIBRARIES})
target_compile_definitions(${PROJECT_NAME}-core PRIVATE -DMELOWN_RENDERER_BUILD_SHARED)



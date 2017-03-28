define_module(LIBRARY ${PROJECT_NAME}-core DEPENDS vts-libs-core http lodepng)

aux_source_directory(. SRC_LIST)
add_library(${PROJECT_NAME}-core STATIC ${SRC_LIST})
buildsys_library(${PROJECT_NAME}-core)
target_link_libraries(${PROJECT_NAME}-core ${MODULE_LIBRARIES})
buildsys_target_compile_definitions(${PROJECT_NAME}-core ${MODULE_DEFINITIONS})

# trick QtCreator into showing some additional files
file(GLOB_RECURSE ADDITIONAL "*.h" "../include/renderer/*.h")
add_custom_target(rendererCoreHeaders SOURCES "${ADDITIONAL}")



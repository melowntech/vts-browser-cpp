define_module(BINARY ${PROJECT_NAME}-core DEPENDS vts-libs-core http lodepng)

aux_source_directory(. SRC_LIST)
add_library(${PROJECT_NAME}-core STATIC ${SRC_LIST})
target_link_libraries(${PROJECT_NAME}-core ${MODULE_LIBRARIES})
target_include_directories(${PROJECT_NAME}-core PUBLIC "../include")

# trick QtCreator into showing some additional files
file(GLOB_RECURSE ADDITIONAL "*.h" "../include/renderer/*.h")
add_custom_target(rendererCoreHeaders SOURCES "${ADDITIONAL}")



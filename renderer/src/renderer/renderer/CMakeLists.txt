define_module(BINARY ${PROJECT_NAME}-core DEPENDS vts-libs-core)

aux_source_directory(. SRC_LIST)
add_library(${PROJECT_NAME}-core STATIC ${SRC_LIST})
target_link_libraries(${PROJECT_NAME}-core ${MODULE_LIBRARIES})

# trick QtCreator into showing some additional files
file(GLOB_RECURSE ADDITIONAL "*.h" "*.ui")
add_custom_target(rendererHeaders SOURCES "${ADDITIONAL}")


define_module(BINARY pokus DEPENDS vts-libs-core)

aux_source_directory(. SRC_LIST)
add_executable(pokus ${SRC_LIST})
target_link_libraries(pokus ${MODULE_LIBRARIES})
target_include_directories(pokus PUBLIC "../include")

# trick QtCreator into showing some additional files
file(GLOB_RECURSE ADDITIONAL "*.h" "../include/renderer/*.h")
add_custom_target(pokusHeaders SOURCES "${ADDITIONAL}")



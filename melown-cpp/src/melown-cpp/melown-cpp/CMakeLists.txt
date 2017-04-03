define_module(LIBRARY melown-cpp DEPENDS vts-libs-core http lodepng)

aux_source_directory(. SRC_LIST)
add_library(melown-cpp STATIC ${SRC_LIST})
buildsys_library(melown-cpp)
target_link_libraries(melown-cpp ${MODULE_LIBRARIES})
buildsys_target_compile_definitions(melown-cpp ${MODULE_DEFINITIONS})

# trick QtCreator into showing some additional files
file(GLOB_RECURSE ADDITIONAL "*.h" "../include/renderer/*.h")
add_custom_target(melownCppHeaders SOURCES "${ADDITIONAL}")



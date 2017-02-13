
# temporarily disable warnings-as-errors
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -w")

# move final outputs into specific directories
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)

# add the projects
add_subdirectory(renderer)
add_subdirectory(qt)

# trick QtCreator into showing some additional files
file(GLOB_RECURSE ADDITIONAL "*.h" "*.ui")
add_custom_target(additional SOURCES "${ADDITIONAL}")


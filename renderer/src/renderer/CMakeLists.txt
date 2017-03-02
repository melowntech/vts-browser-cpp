
# temporarily disable warnings-as-errors
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -w")

# move final outputs into specific directories
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)

# add the projects
add_subdirectory(renderer-core)
if(NOT DEFINED BUILDSYS_NOBUILD_TARGET_renderer-qt)
	add_subdirectory(renderer-qt)
endif()
if(NOT DEFINED BUILDSYS_NOBUILD_TARGET_renderer-glfw)
	add_subdirectory(renderer-glfw)
endif()


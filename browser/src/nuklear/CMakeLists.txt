include_directories(nuklear)
configure_file(nuklear/nuklear.h nuklear.c)
add_library(nuklear STATIC ${CMAKE_CURRENT_BINARY_DIR}/nuklear.c)
buildsys_library(nuklear)
target_include_directories(nuklear SYSTEM PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_compile_definitions(nuklear PRIVATE -DNK_IMPLEMENTATION)
target_compile_options(nuklear PRIVATE -Wno-error -Wno-unused-variable)

set(NUKLEAR_COMPONENTS
	-DNK_INCLUDE_FIXED_TYPES
	-DNK_INCLUDE_STANDARD_IO
	-DNK_INCLUDE_DEFAULT_ALLOCATOR
	-DNK_INCLUDE_VERTEX_BUFFER_OUTPUT
	-DNK_INCLUDE_FONT_BAKING
)

target_compile_definitions(nuklear PUBLIC ${NUKLEAR_COMPONENTS})



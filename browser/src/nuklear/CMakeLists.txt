define_module(LIBRARY nuklear=0 DEPENDS)

include_directories(nuklear)
configure_file(nuklear/nuklear.h nuklear.c)
add_library(nuklear STATIC ${CMAKE_CURRENT_BINARY_DIR}/nuklear.c)
buildsys_library(nuklear)
target_compile_definitions(nuklear PRIVATE -DNK_IMPLEMENTATION)
target_compile_options(nuklear PRIVATE -Wno-error)

set(NUKLEAR_COMPONENTS
	-DNK_INCLUDE_FIXED_TYPES
	-DNK_INCLUDE_STANDARD_IO
	-DNK_INCLUDE_DEFAULT_ALLOCATOR
	-DNK_INCLUDE_VERTEX_BUFFER_OUTPUT
	-DNK_INCLUDE_FONT_BAKING
)
#set(NUKLEAR_COMPONENTS ${NUKLEAR_COMPONENTS} PARENT_SCOPE)

target_compile_definitions(nuklear PUBLIC ${NUKLEAR_COMPONENTS})



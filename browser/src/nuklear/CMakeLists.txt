include_directories(SYSTEM nuklear)
configure_file(nuklear/nuklear.h nuklear.c)
add_library(nuklear STATIC ${CMAKE_CURRENT_BINARY_DIR}/nuklear.c)
buildsys_library(nuklear)
target_include_directories(nuklear SYSTEM PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_compile_definitions(nuklear PRIVATE -DNK_IMPLEMENTATION)
buildsys_ide_groups(nuklear deps)

set(NUKLEAR_COMPONENTS
    -DNK_INCLUDE_FIXED_TYPES
    -DNK_INCLUDE_STANDARD_IO
    -DNK_INCLUDE_DEFAULT_ALLOCATOR
    -DNK_INCLUDE_VERTEX_BUFFER_OUTPUT
    -DNK_INCLUDE_FONT_BAKING
)
target_compile_definitions(nuklear PUBLIC ${NUKLEAR_COMPONENTS})

if(NOT WIN32)
    target_compile_options(nuklear PRIVATE -Wno-error -Wno-unused-variable)
endif()

buildsys_fix_symlinks(${CMAKE_CURRENT_SOURCE_DIR})

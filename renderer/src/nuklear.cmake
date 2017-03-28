define_module(LIBRARY nuklear=0 DEPENDS)

include_directories(src/nuklear)
configure_file(src/nuklear/nuklear.h nuklear.c)
add_library(nuklear STATIC ${CMAKE_CURRENT_BINARY_DIR}/nuklear.c)
buildsys_library(nuklear)
target_compile_definitions(nuklear PRIVATE -DNK_IMPLEMENTATION)



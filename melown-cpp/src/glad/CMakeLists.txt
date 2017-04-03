define_module(LIBRARY glad=0 DEPENDS)

set(glad_SOURCES
	glad.c
)

add_library(glad STATIC ${glad_SOURCES})
buildsys_library(glad)
target_link_libraries(glad ${MODULE_LIBRARIES})
buildsys_target_compile_definitions(glad ${MODULE_DEFINITIONS})




define_module(LIBRARY glad=0.1.13a0 DEPENDS)

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -w")

set(glad_SOURCES
	glad.c
)

add_library(glad STATIC ${glad_SOURCES})
buildsys_library(glad)


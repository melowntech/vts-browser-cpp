define_module(LIBRARY lodepng=20161127 DEPENDS)

set(lodepng_SOURCES
	src/lodepng/lodepng.cpp
)

add_library(lodepng STATIC ${lodepng_SOURCES})
buildsys_library(lodepng)


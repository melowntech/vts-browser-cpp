define_module(LIBRARY lodepng=20161127 DEPENDS)

add_library(lodepng STATIC src/lodepng/lodepng.cpp)
buildsys_library(lodepng)


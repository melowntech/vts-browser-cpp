define_module(LIBRARY lodepng=20161127 DEPENDS)

include_directories(.)
add_library(lodepng STATIC lodepng/lodepng.cpp)
buildsys_library(lodepng)


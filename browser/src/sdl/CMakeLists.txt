define_module(LIBRARY SDL2=0 DEPENDS)

set(SDL_STATIC OFF CACHE BOOL "Build a static version of the library")

add_subdirectory(sdl)

target_compile_options(SDL2     PRIVATE -Wno-error -Wno-unused-parameter -Wno-pedantic -Wno-implicit-function-declaration)



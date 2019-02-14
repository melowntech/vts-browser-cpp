define_module(LIBRARY glad=0 DEPENDS)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -w")
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -w")

set(glad_SOURCES
    glad.h
    glad.c
    KHR/khrplatform.h
)

add_library(glad STATIC ${glad_SOURCES})
buildsys_library(glad)
target_link_libraries(glad ${CMAKE_DL_LIBS})
target_include_directories(glad PUBLIC ..)
buildsys_target_compile_definitions(glad ${MODULE_DEFINITIONS})
buildsys_ide_groups(glad deps)


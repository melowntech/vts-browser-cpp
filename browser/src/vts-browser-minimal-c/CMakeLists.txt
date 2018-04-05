
define_module(BINARY vts-browser-minimal-c DEPENDS
  vts-browser vts-renderer SDL2)

set(SRC_LIST
  main.c
)

add_executable(vts-browser-minimal-c ${SRC_LIST})
target_link_libraries(vts-browser-minimal-c ${MODULE_LIBRARIES})
buildsys_binary(vts-browser-minimal-c)
buildsys_target_compile_definitions(vts-browser-minimal-c ${MODULE_DEFINITIONS})


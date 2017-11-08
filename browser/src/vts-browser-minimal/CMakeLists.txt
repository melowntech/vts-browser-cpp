
define_module(BINARY vts-browser-minimal DEPENDS
  vts-browser vts-renderer SDL2)

set(SRC_LIST
  main.cpp
)

add_executable(vts-browser-minimal ${SRC_LIST})
target_link_libraries(vts-browser-minimal ${MODULE_LIBRARIES})
buildsys_binary(vts-browser-minimal)
buildsys_target_compile_definitions(vts-browser-minimal ${MODULE_DEFINITIONS})


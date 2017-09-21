define_module(BINARY vts-browser-ios DEPENDS
  vts-browser vts-renderer
  glad SDL2 THREADS)

set(HDR_LIST

)

set(SRC_LIST
  main.cpp
)

add_executable(vts-browser-ios ${SRC_LIST} ${HDR_LIST})
target_link_libraries(vts-browser-ios ${MODULE_LIBRARIES})
buildsys_binary(vts-browser-ios)
buildsys_target_compile_definitions(vts-browser-ios ${MODULE_DEFINITIONS})


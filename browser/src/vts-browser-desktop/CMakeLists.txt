
define_module(BINARY vts-browser-desktop DEPENDS
  vts-browser vts-renderer
  nuklear glad SDL2 THREADS Boost_PROGRAM_OPTIONS)

set(SRC_LIST
  dataThread.cpp dataThread.hpp
  guiSkin.cpp guiSkin.hpp
  gui.cpp
  mainWindow.cpp mainWindow.hpp
  programOptions.cpp programOptions.hpp
  main.cpp
)

add_executable(vts-browser-desktop ${SRC_LIST})
target_compile_definitions(vts-browser-desktop PUBLIC ${NUKLEAR_COMPONENTS})
target_link_libraries(vts-browser-desktop ${MODULE_LIBRARIES})
buildsys_binary(vts-browser-desktop)
buildsys_target_compile_definitions(vts-browser-desktop ${MODULE_DEFINITIONS})


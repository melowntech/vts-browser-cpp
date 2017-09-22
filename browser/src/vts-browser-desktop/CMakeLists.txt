
define_module(BINARY vts-browser-desktop DEPENDS
  vts-browser vts-renderer
  nuklear glad SDL2 THREADS Boost_PROGRAM_OPTIONS)

set(HDR_LIST
  dataThread.hpp
  mainWindow.hpp
  guiSkin.hpp
  programOptions.hpp
)

set(SRC_LIST
  dataThread.cpp
  gui.cpp
  guiSkin.cpp
  main.cpp
  mainWindow.cpp
  programOptions.cpp
)

add_executable(vts-browser-desktop ${SRC_LIST} ${HDR_LIST})
target_compile_definitions(vts-browser-desktop PUBLIC ${NUKLEAR_COMPONENTS})
target_link_libraries(vts-browser-desktop ${MODULE_LIBRARIES})
buildsys_binary(vts-browser-desktop)
buildsys_target_compile_definitions(vts-browser-desktop ${MODULE_DEFINITIONS})


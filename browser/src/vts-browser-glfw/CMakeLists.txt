define_module(BINARY vts-browser-glfw DEPENDS vts-browser nuklear glad THREADS Boost_PROGRAM_OPTIONS)

set(HDR_LIST
  dataThread.hpp
  mainWindow.hpp
  gpuContext.hpp
  guiSkin.hpp
  programOptions.hpp
)

set(SRC_LIST
  dataThread.cpp
  gpuContext.cpp
  gui.cpp
  guiSkin.cpp
  main.cpp
  mainWindow.cpp
  programOptions.cpp
)

add_executable(vts-browser-glfw ${SRC_LIST} ${HDR_LIST})
buildsys_binary(vts-browser-glfw)
target_link_libraries(vts-browser-glfw ${MODULE_LIBRARIES} glfw)
buildsys_target_compile_definitions(vts-browser-glfw ${MODULE_DEFINITIONS})
target_compile_definitions(vts-browser-glfw PUBLIC ${NUKLEAR_COMPONENTS})

# install
install(TARGETS vts-browser-glfw
  RUNTIME DESTINATION bin
  COMPONENT browser-glfw
)


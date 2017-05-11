define_module(BINARY vts-browser-glfw DEPENDS vts-browser nuklear glad)

set(HDR_LIST
	dataThread.hpp
	mainWindow.hpp
	gpuContext.hpp
	threadName.hpp
	guiSkin.hpp
)

set(SRC_LIST
	dataThread.cpp
	gpuContext.cpp
	gui.cpp
	guiSkin.cpp
	main.cpp
	mainWindow.cpp
)

add_executable(vts-browser-glfw ${SRC_LIST})
buildsys_binary(vts-browser-glfw)
target_link_libraries(vts-browser-glfw ${MODULE_LIBRARIES} glfw)
buildsys_target_compile_definitions(vts-browser-glfw ${MODULE_DEFINITIONS})

target_compile_definitions(vts-browser-glfw PUBLIC ${NUKLEAR_COMPONENTS})

# trick QtCreator into showing some additional files
add_custom_target(vts-browser-glfw-headers SOURCES "${HDR_LIST}")

# install
install(TARGETS vts-browser-glfw
		RUNTIME DESTINATION bin
		COMPONENT browser-glfw
)


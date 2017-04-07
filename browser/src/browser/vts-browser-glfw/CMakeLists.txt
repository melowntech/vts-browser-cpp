define_module(BINARY vts-browser-glfw DEPENDS vts-browser glad nuklear)

set(HDR_LIST
	dataThread.hpp
	mainWindow.hpp
	gpuContext.hpp
	threadName.hpp
)

set(SRC_LIST
	dataThread.cpp
	gpuContext.cpp
	gui.cpp  main.cpp
	mainWindow.cpp
)

add_executable(vts-browser-glfw ${SRC_LIST})
buildsys_binary(vts-browser-glfw)
target_link_libraries(vts-browser-glfw ${MODULE_LIBRARIES} glfw)
target_include_directories(vts-browser-glfw PUBLIC "../../glfw/include")
buildsys_target_compile_definitions(vts-browser-glfw ${MODULE_DEFINITIONS})

target_compile_definitions(vts-browser-glfw PUBLIC ${NUKLEAR_COMPONENTS})

# trick QtCreator into showing some additional files
add_custom_target(vts-browser-glfw-headers SOURCES "${HDR_LIST}")

# install
install(TARGETS vts-browser-glfw
			RUNTIME DESTINATION bin
			COMPONENT browser-glfw
)


define_module(BINARY melown-glfw DEPENDS melown-cpp glad nuklear)

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

add_executable(melown-glfw ${SRC_LIST})
buildsys_binary(melown-glfw)
target_link_libraries(melown-glfw ${MODULE_LIBRARIES} glfw)
target_include_directories(melown-glfw PUBLIC "../../glfw/include")
buildsys_target_compile_definitions(melown-glfw ${MODULE_DEFINITIONS})

target_compile_definitions(melown-glfw PUBLIC ${NUKLEAR_COMPONENTS})

# trick QtCreator into showing some additional files
add_custom_target(melownGlfwHeaders SOURCES "${HDR_LIST}")



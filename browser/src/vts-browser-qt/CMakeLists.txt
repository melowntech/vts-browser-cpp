# QT has some problems with COMPILE_DEFINITIONS_*
cmake_policy(SET CMP0043 OLD)

define_module(BINARY vts-browser-qt DEPENDS vts-browser)

message(STATUS "configuring vts-browser-qt")

set(HDR_LIST
	fetcher.hpp
	gpuContext.hpp
	mainWindow.hpp
)

set(SRC_LIST
	fetcher.cpp
	gpuContext.cpp
	main.cpp
	mainWindow.cpp
)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)

add_executable(vts-browser-qt ${SRC_LIST} ${HDR_LIST})
buildsys_binary(vts-browser-qt)
target_link_libraries(vts-browser-qt ${MODULE_LIBRARIES})
buildsys_target_compile_definitions(vts-browser-qt ${MODULE_DEFINITIONS})
qt5_use_modules(vts-browser-qt Gui Network)

# install
install(TARGETS vts-browser-qt
			RUNTIME DESTINATION bin
			COMPONENT browser-qt
)


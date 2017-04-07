define_module(BINARY vts-browser-qt DEPENDS vts-browser-cpp)

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

find_package(Qt5Gui REQUIRED)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)

add_executable(vts-browser-qt ${SRC_LIST})
buildsys_binary(vts-browser-qt)
target_link_libraries(vts-browser-qt ${MODULE_LIBRARIES})
buildsys_target_compile_definitions(vts-browser-qt ${MODULE_DEFINITIONS})
qt5_use_modules(vts-browser-qt Gui Network)

# trick QtCreator into showing some additional files
add_custom_target(vts-browser-qt-headers SOURCES "${HDR_LIST}")



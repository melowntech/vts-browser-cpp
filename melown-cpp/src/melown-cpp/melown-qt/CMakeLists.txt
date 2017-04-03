define_module(BINARY melown-qt DEPENDS melown-cpp)

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

add_executable(melown-qt ${SRC_LIST})
buildsys_binary(melown-qt)
target_link_libraries(melown-qt ${MODULE_LIBRARIES})
buildsys_target_compile_definitions(melown-qt ${MODULE_DEFINITIONS})
qt5_use_modules(melown-qt Gui Network)

# trick QtCreator into showing some additional files
add_custom_target(melownQtHeaders SOURCES "${HDR_LIST}")



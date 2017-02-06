define_module(BINARY desktop DEPENDS vts-libs-core)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_INCLUDE_CURRENT_DIR ON)
find_package(Qt5Widgets REQUIRED)


aux_source_directory(. SRC_LIST)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
add_executable(desktop ${SRC_LIST})
target_link_libraries(desktop ${MODULE_LIBRARIES})
qt5_use_modules(desktop Widgets)


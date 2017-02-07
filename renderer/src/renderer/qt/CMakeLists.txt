define_module(BINARY ${PROJECT_NAME} DEPENDS ${PROJECT_NAME}-core)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_INCLUDE_CURRENT_DIR ON)
find_package(Qt5Widgets REQUIRED)
aux_source_directory(. SRC_LIST)
add_executable(${PROJECT_NAME} ${SRC_LIST})
target_include_directories(${PROJECT_NAME} PRIVATE ../include)
target_link_libraries(${PROJECT_NAME} ${MODULE_LIBRARIES})
qt5_use_modules(${PROJECT_NAME} Widgets)


define_module(BINARY ${PROJECT_NAME}-qt DEPENDS ${PROJECT_NAME}-core)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_INCLUDE_CURRENT_DIR ON)
find_package(Qt5Gui REQUIRED)
aux_source_directory(. SRC_LIST)
add_executable(${PROJECT_NAME}-qt ${SRC_LIST})
buildsys_binary(${PROJECT_NAME}-qt)
target_link_libraries(${PROJECT_NAME}-qt ${MODULE_LIBRARIES})
buildsys_target_compile_definitions(${PROJECT_NAME}-qt ${MODULE_DEFINITIONS})
qt5_use_modules(${PROJECT_NAME}-qt Gui Network)

# trick QtCreator into showing some additional files
file(GLOB_RECURSE ADDITIONAL "*.h" "*.ui")
add_custom_target(rendererQtHeaders SOURCES "${ADDITIONAL}")



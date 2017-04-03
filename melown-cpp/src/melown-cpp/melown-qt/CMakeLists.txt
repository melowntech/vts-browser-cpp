define_module(BINARY melown-qt DEPENDS melown-cpp)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_INCLUDE_CURRENT_DIR ON)
find_package(Qt5Gui REQUIRED)
aux_source_directory(. SRC_LIST)
add_executable(melown-qt ${SRC_LIST})
buildsys_binary(melown-qt)
target_link_libraries(melown-qt ${MODULE_LIBRARIES})
buildsys_target_compile_definitions(melown-qt ${MODULE_DEFINITIONS})
qt5_use_modules(melown-qt Gui Network)

# trick QtCreator into showing some additional files
file(GLOB_RECURSE ADDITIONAL "*.h" "*.ui")
add_custom_target(melownQtHeaders SOURCES "${ADDITIONAL}")



define_module(BINARY ${PROJECT_NAME}-glfw DEPENDS ${PROJECT_NAME}-core glad)

set(CMAKE_INCLUDE_CURRENT_DIR ON)
aux_source_directory(. SRC_LIST)
add_executable(${PROJECT_NAME}-glfw ${SRC_LIST})
target_link_libraries(${PROJECT_NAME}-glfw ${MODULE_LIBRARIES} glfw ${GLFW_LIBRARIES})
target_include_directories(${PROJECT_NAME}-glfw PUBLIC "../../glfw/include")

# trick QtCreator into showing some additional files
file(GLOB_RECURSE ADDITIONAL "*.h")
add_custom_target(rendererGlfwHeaders SOURCES "${ADDITIONAL}")



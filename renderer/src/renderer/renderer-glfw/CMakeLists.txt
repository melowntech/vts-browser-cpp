define_module(BINARY ${PROJECT_NAME}-glfw DEPENDS ${PROJECT_NAME}-core glad nuklear)

set(CMAKE_INCLUDE_CURRENT_DIR ON)
aux_source_directory(. SRC_LIST)
add_executable(${PROJECT_NAME}-glfw ${SRC_LIST})
buildsys_binary(${PROJECT_NAME}-glfw)
target_link_libraries(${PROJECT_NAME}-glfw ${MODULE_LIBRARIES} glfw)
target_include_directories(${PROJECT_NAME}-glfw PUBLIC "../../glfw/include")
buildsys_target_compile_definitions(${PROJECT_NAME}-glfw ${MODULE_DEFINITIONS})

target_compile_definitions(${PROJECT_NAME}-glfw PUBLIC ${NUKLEAR_COMPONENTS})

# trick QtCreator into showing some additional files
file(GLOB_RECURSE ADDITIONAL "*.h")
add_custom_target(rendererGlfwHeaders SOURCES "${ADDITIONAL}")



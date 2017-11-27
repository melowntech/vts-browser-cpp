define_module(LIBRARY vts-renderer DEPENDS vts-browser glad)

set(PUB_HDR_LIST
    include/vts-renderer/foundation.hpp
    include/vts-renderer/renderer.hpp
    include/vts-renderer/classes.hpp
)

set(SRC_LIST
    renderer.cpp renderer.hpp
    classes.cpp
    errors.cpp
)

set(DATA_LIST
    data/meshes/aabb.obj
    data/meshes/cube.obj
    data/meshes/line.obj
    data/meshes/quad.obj
    data/meshes/rect.obj
    data/meshes/sphere.obj
    data/shaders/atmosphere.frag.glsl
    data/shaders/atmosphere.vert.glsl
    data/shaders/surface.frag.glsl
    data/shaders/surface.vert.glsl
    data/shaders/infographic.frag.glsl
    data/shaders/infographic.vert.glsl
    data/shaders/gui.frag.glsl
    data/shaders/gui.vert.glsl
    data/shaders/scales.frag.glsl
    data/shaders/scales.vert.glsl
    data/shaders/copyDepth.frag.glsl
    data/shaders/copyDepth.vert.glsl
    data/textures/helper.jpg
)

pack_data(initializeRenderData)
add_library(vts-renderer ${LIB_TYPE} ${SRC_LIST} ${PUB_HDR_LIST})
target_compile_definitions(vts-renderer ${LIB_VIS} VTSR_BUILD_${LIB_TYPE})
target_compile_definitions(vts-renderer PRIVATE VTSR_INCLUDE_GL)
target_link_libraries(vts-renderer ${LIB_VIS} ${MODULE_LIBRARIES})
target_link_libraries(vts-renderer PRIVATE initializeRenderData)
buildsys_library(vts-renderer)
buildsys_soversion(vts-renderer ${vts-browser_SO_VERSION})
buildsys_target_compile_definitions(vts-renderer ${MODULE_DEFINITIONS})
buildsys_position_independent(${MODULE_LIBRARIES})

if(IOS)
    target_compile_definitions(vts-renderer PRIVATE VTSR_OPENGLES)
endif()



#######################################################################
# Install

set(VTSRENDERER_LIBRARIES vts-renderer)

# headers
install(FILES ${PUB_HDR_LIST}
    DESTINATION ${INCLUDE_INSTALL_DIR}/vts-renderer
    COMPONENT renderer-dev
)

# library and binaries
install(TARGETS vts-renderer
    COMPONENT renderer
    EXPORT vts-renderer
    RUNTIME DESTINATION ${BIN_INSTALL_DIR}
    LIBRARY DESTINATION ${LIB_INSTALL_DIR}
    ARCHIVE DESTINATION ${LIB_INSTALL_DIR}
)

install(EXPORT vts-renderer
    DESTINATION ${SHARE_INSTALL_DIR}/VtsRenderer
    COMPONENT renderer-dev
)

# VtsRendererConfig.cmake and VtsRendererVersion.cmake
include(CMakePackageConfigHelpers)

configure_package_config_file(
    VtsRendererConfig.cmake.in
    ${CMAKE_CURRENT_BINARY_DIR}/VtsRendererConfig.cmake
    INSTALL_DESTINATION ${SHARE_INSTALL_DIR}/VtsRenderer
    PATH_VARS INCLUDE_INSTALL_DIR
)

write_basic_package_version_file(
    ${CMAKE_CURRENT_BINARY_DIR}/VtsRendererConfigVersion.cmake
    VERSION ${vts-browser_SO_VERSION}
    COMPATIBILITY SameMajorVersion
)

install(FILES
    ${CMAKE_CURRENT_BINARY_DIR}/VtsRendererConfig.cmake
    ${CMAKE_CURRENT_BINARY_DIR}/VtsRendererConfigVersion.cmake
    DESTINATION ${SHARE_INSTALL_DIR}/VtsRenderer
    COMPONENT renderer-dev
)





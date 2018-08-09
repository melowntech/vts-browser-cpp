define_module(LIBRARY vts-renderer DEPENDS vts-browser glad)

set(PUB_HDR_LIST
    include/vts-renderer/foundation_common.h
    include/vts-renderer/foundation.h
    include/vts-renderer/foundation.hpp
    include/vts-renderer/renderer_common.h
    include/vts-renderer/renderer.h
    include/vts-renderer/renderer.hpp
    include/vts-renderer/classes.hpp
)

set(SRC_LIST
    renderer.cpp renderer.hpp
    renderer_c.cpp
    classes.cpp
    foundation.cpp
)

set(DATA_LIST
    data/shaders/atmosphere.inc.glsl
    data/shaders/background.vert.glsl
    data/shaders/background.frag.glsl
    data/shaders/surface.frag.glsl
    data/shaders/surface.vert.glsl
    data/shaders/infographic.frag.glsl
    data/shaders/infographic.vert.glsl
    data/shaders/texture.frag.glsl
    data/shaders/texture.vert.glsl
    data/shaders/color.frag.glsl
    data/shaders/color.vert.glsl
    data/shaders/copyDepth.frag.glsl
    data/shaders/copyDepth.vert.glsl
    data/textures/compas.png
)

buildsys_pack_data(initializeRenderData)
if(BUILDSYS_EMBEDDED AND NOT BUILD_FRAMEWORK_ONLY)
    add_library(vts-renderer STATIC ${SRC_LIST} ${PUB_HDR_LIST})
    target_compile_definitions(vts-renderer PUBLIC VTSR_BUILD_STATIC)
    target_compile_definitions(vts-renderer PRIVATE VTSR_INCLUDE_GL)
    target_link_libraries(vts-renderer PUBLIC initializeRenderData ${MODULE_LIBRARIES})
else()
    add_library(vts-renderer SHARED ${SRC_LIST} ${PUB_HDR_LIST})
    target_compile_definitions(vts-renderer PRIVATE VTSR_BUILD_SHARED VTSR_INCLUDE_GL)
    target_link_libraries(vts-renderer PRIVATE initializeRenderData ${MODULE_LIBRARIES})
    set_target_properties(vts-renderer PROPERTIES FRAMEWORK TRUE)
    set_target_properties(vts-renderer PROPERTIES PUBLIC_HEADER "${PUB_HDR_LIST}")
endif()
buildsys_library(vts-renderer)
buildsys_soversion(vts-renderer ${vts-browser_SO_VERSION})
buildsys_target_compile_definitions(vts-renderer ${MODULE_DEFINITIONS})
buildsys_position_independent(${MODULE_LIBRARIES})
buildsys_ide_groups(vts-renderer libs)

if(BUILDSYS_EMBEDDED)
    target_compile_definitions(vts-renderer PRIVATE VTSR_OPENGLES)
endif()



#######################################################################
# Install

if(NOT BUILDSYS_EMBEDDED)

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
    FRAMEWORK DESTINATION ${LIB_INSTALL_DIR}
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

endif()



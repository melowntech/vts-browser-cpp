
find_package(Freetype REQUIRED)
include_directories(SYSTEM ${FREETYPE_INCLUDE_DIRS})
find_package(HarfBuzz REQUIRED)
include_directories(SYSTEM "${HARFBUZZ_INCLUDE_DIR}")
include_directories(SYSTEM "${HARFBUZZ_INCLUDE_DIR}/harfbuzz")
include_directories(externals/utility)

buildsys_fix_symlinks(${CMAKE_CURRENT_SOURCE_DIR}/externals)
add_subdirectory(externals/sheenbidi)

define_module(LIBRARY vts-renderer DEPENDS vts-browser utility glad FREETYPE HarfBuzz SheenBidi utf8cpp)

set(PUB_HDR_LIST
    include/vts-renderer/foundationCommon.h
    include/vts-renderer/foundation.h
    include/vts-renderer/foundation.hpp
    include/vts-renderer/rendererCommon.h
    include/vts-renderer/renderer.h
    include/vts-renderer/renderer.hpp
    include/vts-renderer/classes.hpp
)

set(SRC_LIST
    classes.cpp
    depthBuffer.cpp
    font.cpp
    font.hpp
    foundation.cpp
    geodata.cpp
    geodata.hpp
    geodataGeometry.cpp
    geodataText.cpp
    renderer.hpp
    rendererApiC.cpp
    rendererApiCpp.cpp
    renderContext.cpp
    renderView.cpp
    shapes.cpp
    shapes.hpp
)

set(DATA_LIST
    data/shaders/atmosphere.inc.glsl
    data/shaders/background.frag.glsl
    data/shaders/background.vert.glsl
    data/shaders/color.frag.glsl
    data/shaders/color.vert.glsl
    data/shaders/copyDepth.frag.glsl
    data/shaders/copyDepth.vert.glsl
    data/shaders/geodata.inc.glsl
    data/shaders/geodataColor.frag.glsl
    data/shaders/geodataColor.vert.glsl
    data/shaders/geodataIcon.frag.glsl
    data/shaders/geodataIcon.vert.glsl
    data/shaders/geodataLabelFlat.frag.glsl
    data/shaders/geodataLabelFlat.vert.glsl
    data/shaders/geodataLabelScreen.frag.glsl
    data/shaders/geodataLabelScreen.vert.glsl
    data/shaders/geodataLine.frag.glsl
    data/shaders/geodataLineFlat.vert.glsl
    data/shaders/geodataLineScreen.vert.glsl
    data/shaders/geodataPoint.frag.glsl
    data/shaders/geodataPointFlat.vert.glsl
    data/shaders/geodataPointScreen.vert.glsl
    data/shaders/geodataTriangle.frag.glsl
    data/shaders/geodataTriangle.vert.glsl
    data/shaders/infographics.frag.glsl
    data/shaders/infographics.vert.glsl
    data/shaders/surface.frag.glsl
    data/shaders/surface.vert.glsl
    data/shaders/texture.frag.glsl
    data/shaders/texture.vert.glsl
    data/textures/compas.png
    data/textures/blueNoise/0.png
    data/textures/blueNoise/1.png
    data/textures/blueNoise/2.png
    data/textures/blueNoise/3.png
    data/textures/blueNoise/4.png
    data/textures/blueNoise/5.png
    data/textures/blueNoise/6.png
    data/textures/blueNoise/7.png
    data/textures/blueNoise/8.png
    data/textures/blueNoise/9.png
    data/textures/blueNoise/10.png
    data/textures/blueNoise/11.png
    data/textures/blueNoise/12.png
    data/textures/blueNoise/13.png
    data/textures/blueNoise/14.png
    data/textures/blueNoise/15.png
)

buildsys_pack_data(initializeRenderData)
add_library(vts-renderer ${VTS_BROWSER_BUILD_LIBRARY} ${SRC_LIST} ${PUB_HDR_LIST} ${DATA_LIST})
target_link_libraries(vts-renderer ${VTS_BROWSER_BUILD_VISIBILITY} initializeRenderData ${MODULE_LIBRARIES} ${HARFBUZZ_LIBRARIES})
target_link_libraries(vts-renderer PRIVATE Optick)
target_compile_definitions(vts-renderer ${VTS_BROWSER_BUILD_VISIBILITY} VTSR_BUILD_${VTS_BROWSER_BUILD_MACRO})
target_compile_definitions(vts-renderer PRIVATE VTSR_INCLUDE_GL ${MODULE_DEFINITIONS})
set_target_properties(vts-renderer PROPERTIES
    MACOSX_FRAMEWORK_IDENTIFIER com.melown.vts-renderer
    MACOSX_BUNDLE_BUNDLE_NAME vts-renderer
    MACOSX_BUNDLE_BUNDLE_VERSION ${vts-browser_SO_VERSION}
    PUBLIC_HEADER "${PUB_HDR_LIST}"
)
if(VTS_BROWSER_TYPE STREQUAL "FRAMEWORK")
    set_target_properties(vts-renderer PROPERTIES FRAMEWORK TRUE)
endif()
if(VTS_BROWSER_TYPE STREQUAL "MODULE")
    set_target_properties(vts-renderer PROPERTIES BUNDLE TRUE)
endif()
buildsys_library(vts-renderer)
buildsys_soversion(vts-renderer ${vts-browser_SO_VERSION})
buildsys_position_independent(${MODULE_LIBRARIES})
buildsys_ide_groups(vts-renderer libs)

if(BUILDSYS_EMBEDDED)
    target_compile_definitions(vts-renderer PRIVATE VTSR_EMBEDDED)
    target_compile_definitions(vts-renderer PRIVATE VTSR_OPENGLES)
endif()
if(BUILDSYS_IOS)
    target_compile_definitions(vts-renderer PRIVATE VTSR_IOS)
endif()
if(BUILDSYS_UWP)
    target_compile_definitions(vts-renderer PRIVATE VTSR_UWP)
    target_compile_definitions(vts-renderer PRIVATE VTSR_NO_CLIP)
endif()




#######################################################################
# Install

if(NOT BUILDSYS_EMBEDDED)

include(GNUInstallDirs)

set(VTSRENDERER_LIBRARIES vts-renderer)

# library and binaries
install(TARGETS vts-renderer
    COMPONENT renderer
    EXPORT vts-renderer
    PUBLIC_HEADER DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/vts-renderer
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
    FRAMEWORK DESTINATION ${CMAKE_INSTALL_LIBDIR}
)

install(EXPORT vts-renderer
    DESTINATION ${CMAKE_INSTALL_DATADIR}/VtsRenderer
    COMPONENT renderer-dev
)

# VtsRendererConfig.cmake and VtsRendererVersion.cmake
include(CMakePackageConfigHelpers)

set(INCLUDE_INSTALL_DIR ${CMAKE_INSTALL_INCLUDEDIR})

configure_package_config_file(
    VtsRendererConfig.cmake.in
    ${CMAKE_CURRENT_BINARY_DIR}/VtsRendererConfig.cmake
    INSTALL_DESTINATION ${CMAKE_INSTALL_DATADIR}/VtsRenderer
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
    DESTINATION ${CMAKE_INSTALL_DATADIR}/VtsRenderer
    COMPONENT renderer-dev
)

endif()



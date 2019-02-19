
find_package(Freetype REQUIRED)
include_directories(SYSTEM ${FREETYPE_INCLUDE_DIR})
find_package(HarfBuzz REQUIRED)
include_directories(SYSTEM ${HARFBUZZ_INCLUDE_DIR})
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
    font.cpp
    font.hpp
    foundation.cpp
    geodata.cpp
    renderer.cpp
    renderer.hpp
    rendererApiC.cpp
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
    data/shaders/geodataLine.frag.glsl
    data/shaders/geodataLine.vert.glsl
    data/shaders/geodataPoint.frag.glsl
    data/shaders/geodataPoint.vert.glsl
    data/textures/compas.png
)

buildsys_pack_data(initializeRenderData)
add_library(vts-renderer ${VTS_BROWSER_BUILD_LIBRARY} ${SRC_LIST} ${PUB_HDR_LIST} ${DATA_LIST})
target_link_libraries(vts-renderer ${VTS_BROWSER_BUILD_VISIBILITY} initializeRenderData ${MODULE_LIBRARIES})
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
    target_compile_definitions(vts-renderer PRIVATE VTSR_OPENGLES)
endif()




#######################################################################
# Install

if(NOT BUILDSYS_EMBEDDED)

include(GNUInstallDirs)

set(VTSRENDERER_LIBRARIES vts-renderer)

# headers
install(FILES ${PUB_HDR_LIST}
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/vts-renderer
    COMPONENT renderer-dev
)

# library and binaries
install(TARGETS vts-renderer
    COMPONENT renderer
    EXPORT vts-renderer
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



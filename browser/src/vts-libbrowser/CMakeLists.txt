
set(EXTRA_SRC_LIST)
set(EXTRA_LIB_DEPS)
set(EXTRA_LIB_MODULES)

if(BUILDSYS_IOS)

    # ios
    list(APPEND EXTRA_LIB_DEPS iconv objc "-framework CoreGraphics")
    list(APPEND EXTRA_SRC_LIST fetcher/ios.mm)

else()

    # desktop
    list(APPEND EXTRA_LIB_MODULES http)
    list(APPEND EXTRA_SRC_LIST fetcher/curl.cpp)

endif()

find_package(ZLIB REQUIRED)
find_package(PNG REQUIRED)
find_package(JPEG REQUIRED)
include_directories(SYSTEM ${ZLIB_INCLUDE_DIRS})
include_directories(SYSTEM ${PNG_INCLUDE_DIRS})
include_directories(SYSTEM ${JPEG_INCLUDE_DIRS})

define_module(LIBRARY vts-browser DEPENDS vts-libs-nucleus
    jsoncpp ZLIB PNG JPEG ${EXTRA_LIB_MODULES})

set(PUB_HDR_LIST
    # C/C++ API
    include/vts-browser/foundation_common.h
    include/vts-browser/draws_common.h
    # C++ API
    include/vts-browser/boostProgramOptions.hpp
    include/vts-browser/buffer.hpp
    include/vts-browser/callbacks.hpp
    include/vts-browser/celestial.hpp
    include/vts-browser/credits.hpp
    include/vts-browser/draws.hpp
    include/vts-browser/exceptions.hpp
    include/vts-browser/fetcher.hpp
    include/vts-browser/foundation.hpp
    include/vts-browser/log.hpp
    include/vts-browser/map.hpp
    include/vts-browser/math.hpp
    include/vts-browser/options.hpp
    include/vts-browser/resources.hpp
    include/vts-browser/search.hpp
    include/vts-browser/statistics.hpp
    include/vts-browser/view.hpp
    # C API
    include/vts-browser/callbacks.h
    include/vts-browser/celestial.h
    include/vts-browser/draws.h
    include/vts-browser/fetcher.h
    include/vts-browser/foundation.h
    include/vts-browser/log.h
    include/vts-browser/map.h
    include/vts-browser/math.h
    include/vts-browser/resources.h
    include/vts-browser/search.h
)

set(SRC_LIST
    resources/auth.cpp
    resources/cache.hpp
    resources/cache.cpp
    resources/fetcher.cpp
    resources/geodata.cpp
    resources/manager.cpp
    resources/mapConfig.cpp
    resources/mesh.cpp
    resources/other.cpp
    resources/resource.cpp
    resources/resources.cpp
    resources/texture.cpp
    image/image.hpp
    image/image.cpp
    image/jpeg.cpp
    image/png.cpp
    utilities/array.hpp
    utilities/json.hpp
    utilities/json.cpp
    utilities/obj.hpp
    utilities/obj.cpp
    utilities/threadName.hpp
    utilities/threadName.cpp
    utilities/threadQueue.hpp
    boostProgramOptions.cpp
    boundLayers.cpp
    buffer.cpp
    coordsManip.hpp
    coordsManip.cpp
    credits.hpp
    credits.cpp
    exceptions.cpp
    log.cpp
    map.hpp
    mapApiC.hpp
    mapApiC.cpp
    mapApiCpp.cpp
    mapCelestialBody.cpp
    mapDraws.cpp
    mapLayer.cpp
    mapUtilities.cpp
    math.cpp
    mathColor.cpp
    navigation.cpp
    navigationAltitude.cpp
    navigationPiha.hpp
    navigationPiha.cpp
    options.cpp
    renderer.cpp
    rendererTraversal.cpp
    search.cpp
    sri.cpp
    statistics.cpp
    surfaceStack.cpp
    traverseNode.cpp
    ${EXTRA_SRC_LIST}
)

set(DATA_LIST
    data/meshes/aabb.obj
    data/meshes/cube.obj
    data/meshes/line.obj
    data/meshes/quad.obj
    data/meshes/rect.obj
    data/meshes/sphere.obj
    data/textures/helper.jpg
)

buildsys_pack_data(initializeBrowserData)
add_library(vts-browser ${LIB_TYPE} ${SRC_LIST} ${PUB_HDR_LIST})
target_compile_definitions(vts-browser ${LIB_VIS} VTS_BROWSER_BUILD_${LIB_TYPE})
target_link_libraries(vts-browser ${LIB_VIS} ${MODULE_LIBRARIES} ${EXTRA_LIB_DEPS})
target_link_libraries(vts-browser PRIVATE initializeBrowserData)
buildsys_library(vts-browser)
buildsys_soversion(vts-browser ${vts-browser_SO_VERSION})
buildsys_target_compile_definitions(vts-browser ${MODULE_DEFINITIONS})
buildsys_position_independent(${MODULE_LIBRARIES})
buildsys_ide_groups(vts-browser libs)

option(VTS_ENABLE_FREE_LAYERS "compile in support for free layers [WIP]")
if(VTS_ENABLE_FREE_LAYERS)
    target_compile_definitions(vts-browser PRIVATE VTS_ENABLE_FREE_LAYERS)
endif()

#######################################################################
# Install

if(NOT BUILDSYS_EMBEDDED)

set(VTSBROWSER_LIBRARIES vts-browser)

# headers
install(FILES ${PUB_HDR_LIST}
    DESTINATION ${INCLUDE_INSTALL_DIR}/vts-browser
    COMPONENT browser-dev
)

# library and binaries
install(TARGETS vts-browser
    COMPONENT browser
    EXPORT vts-browser
    RUNTIME DESTINATION ${BIN_INSTALL_DIR}
    LIBRARY DESTINATION ${LIB_INSTALL_DIR}
    ARCHIVE DESTINATION ${LIB_INSTALL_DIR}
)

install(EXPORT vts-browser
    DESTINATION ${SHARE_INSTALL_DIR}/VtsBrowser
    COMPONENT browser-dev
)

# VtsBrowserConfig.cmake and VtsBrowserVersion.cmake
include(CMakePackageConfigHelpers)

configure_package_config_file(
    VtsBrowserConfig.cmake.in
    ${CMAKE_CURRENT_BINARY_DIR}/VtsBrowserConfig.cmake
    INSTALL_DESTINATION ${SHARE_INSTALL_DIR}/VtsBrowser
    PATH_VARS INCLUDE_INSTALL_DIR
)

write_basic_package_version_file(
    ${CMAKE_CURRENT_BINARY_DIR}/VtsBrowserConfigVersion.cmake
    VERSION ${vts-browser_SO_VERSION}
    COMPATIBILITY SameMajorVersion
)

install(FILES
    ${CMAKE_CURRENT_BINARY_DIR}/VtsBrowserConfig.cmake
    ${CMAKE_CURRENT_BINARY_DIR}/VtsBrowserConfigVersion.cmake
    DESTINATION ${SHARE_INSTALL_DIR}/VtsBrowser
    COMPONENT browser-dev
)

endif()


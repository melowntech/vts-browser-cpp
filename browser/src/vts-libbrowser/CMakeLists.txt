
set(EXTRA_SRC_LIST)
set(EXTRA_LIB_DEPS)
set(EXTRA_LIB_MODULES)

if(BUILDSYS_IOS)
    # ios
    list(APPEND EXTRA_LIB_DEPS iconv objc "-framework CoreGraphics")
    list(APPEND EXTRA_SRC_LIST fetcher/ios.mm)
else()
    # desktop
    buildsys_fix_symlinks(${CMAKE_CURRENT_SOURCE_DIR}/fetcher)
    find_package(CURL REQUIRED)
    include_directories(SYSTEM ${CURL_INCLUDE_DIR})
    add_subdirectory(fetcher/http)
    include_directories(SYSTEM fetcher)
    buildsys_ide_groups(http deps)
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
    atmosphereDensityTexture.cpp
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
add_library(vts-browser ${VTS_BROWSER_BUILD_LIBRARY} ${SRC_LIST} ${PUB_HDR_LIST} ${DATA_LIST})
target_compile_definitions(vts-browser ${VTS_BROWSER_BUILD_VISIBILITY} VTS_BUILD_${VTS_BROWSER_BUILD_MACRO})
target_compile_definitions(vts-browser PRIVATE ${MODULE_DEFINITIONS})
target_link_libraries(vts-browser ${VTS_BROWSER_BUILD_VISIBILITY} initializeBrowserData ${MODULE_LIBRARIES} ${EXTRA_LIB_DEPS})
set_target_properties(vts-browser PROPERTIES
    MACOSX_FRAMEWORK_IDENTIFIER com.melown.vts-browser
    MACOSX_BUNDLE_BUNDLE_NAME vts-browser
    MACOSX_BUNDLE_BUNDLE_VERSION ${vts-browser_SO_VERSION}
    PUBLIC_HEADER "${PUB_HDR_LIST}"
)
if(VTS_BROWSER_TYPE STREQUAL "FRAMEWORK")
    set_target_properties(vts-browser PROPERTIES FRAMEWORK TRUE)
endif()
if(VTS_BROWSER_TYPE STREQUAL "MODULE")
    set_target_properties(vts-browser PROPERTIES BUNDLE TRUE)
endif()
buildsys_library(vts-browser)
buildsys_soversion(vts-browser ${vts-browser_SO_VERSION})
buildsys_position_independent(${MODULE_LIBRARIES})
buildsys_ide_groups(vts-browser libs)

option(VTS_ENABLE_FREE_LAYERS "compile in support for free layers [WIP]")
if(VTS_ENABLE_FREE_LAYERS)
    target_compile_definitions(vts-browser PRIVATE VTS_ENABLE_FREE_LAYERS)
endif()



#######################################################################
# Install

if(NOT BUILDSYS_EMBEDDED)

include(GNUInstallDirs)

set(VTSBROWSER_LIBRARIES vts-browser)

# headers
install(FILES ${PUB_HDR_LIST}
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/vts-browser
    COMPONENT browser-dev
)

# library and binaries
install(TARGETS vts-browser
    COMPONENT browser
    EXPORT vts-browser
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
    FRAMEWORK DESTINATION ${CMAKE_INSTALL_LIBDIR}
)

install(EXPORT vts-browser
    DESTINATION ${CMAKE_INSTALL_DATADIR}/VtsBrowser
    COMPONENT browser-dev
)

# VtsBrowserConfig.cmake and VtsBrowserVersion.cmake
include(CMakePackageConfigHelpers)

set(INCLUDE_INSTALL_DIR ${CMAKE_INSTALL_INCLUDEDIR})

configure_package_config_file(
    VtsBrowserConfig.cmake.in
    ${CMAKE_CURRENT_BINARY_DIR}/VtsBrowserConfig.cmake
    INSTALL_DESTINATION ${CMAKE_INSTALL_DATADIR}/VtsBrowser
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
    DESTINATION ${CMAKE_INSTALL_DATADIR}/VtsBrowser
    COMPONENT browser-dev
)

endif()


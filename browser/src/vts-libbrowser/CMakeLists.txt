
include_directories(SYSTEM externals)
add_subdirectory(externals)

find_package(ZLIB REQUIRED)
include_directories(SYSTEM ${ZLIB_INCLUDE_DIRS})
find_package(PNG REQUIRED)
include_directories(SYSTEM ${PNG_INCLUDE_DIRS})
find_package(JPEG REQUIRED)
include_directories(SYSTEM ${JPEG_INCLUDE_DIRS})

set(EXTRA_SRC_LIST)
set(EXTRA_LIB_DEPS)
set(EXTRA_LIB_MODULES)

if(BUILDSYS_IOS)
    # ios
    list(APPEND EXTRA_LIB_DEPS iconv objc "-framework CoreGraphics")
    list(APPEND EXTRA_SRC_LIST fetcher/ios.mm)
elseif(BUILDSYS_UWP)
    # universal windows platform
    list(APPEND EXTRA_SRC_LIST fetcher/uwp.cpp)
    set_source_files_properties(fetcher/uwp.cpp PROPERTIES COMPILE_FLAGS "/ZW /Zc:twoPhase-")
elseif(BUILDSYS_WASM)
    # web assembly
    list(APPEND EXTRA_SRC_LIST fetcher/wasm.cpp)
else()
    # desktop
    buildsys_fix_symlinks(${CMAKE_CURRENT_SOURCE_DIR}/fetcher)
    find_package(CURL REQUIRED)
    include_directories(SYSTEM ${CURL_INCLUDE_DIR})
    include_directories(SYSTEM fetcher)
    add_subdirectory(fetcher/http)
    buildsys_ide_groups(http deps)
    list(APPEND EXTRA_LIB_MODULES http)
    list(APPEND EXTRA_SRC_LIST fetcher/curl.cpp)
endif()

define_module(LIBRARY vts-browser DEPENDS vts-libs-nucleus
    jsoncpp ZLIB PNG JPEG utf8cpp ${EXTRA_LIB_MODULES})

set(PUB_HDR_LIST
    # C/C++ API
    include/vts-browser/foundationCommon.h
    include/vts-browser/cameraCommon.h
    include/vts-browser/positionCommon.h
    # C++ API
    include/vts-browser/boostProgramOptions.hpp
    include/vts-browser/buffer.hpp
    include/vts-browser/camera.hpp
    include/vts-browser/cameraCredits.hpp
    include/vts-browser/cameraDraws.hpp
    include/vts-browser/cameraOptions.hpp
    include/vts-browser/cameraStatistics.hpp
    include/vts-browser/celestial.hpp
    include/vts-browser/exceptions.hpp
    include/vts-browser/fetcher.hpp
    include/vts-browser/foundation.hpp
    include/vts-browser/geodata.hpp
    include/vts-browser/log.hpp
    include/vts-browser/map.hpp
    include/vts-browser/mapCallbacks.hpp
    include/vts-browser/mapOptions.hpp
    include/vts-browser/mapStatistics.hpp
    include/vts-browser/math.hpp
    include/vts-browser/navigation.hpp
    include/vts-browser/navigationOptions.hpp
    include/vts-browser/perfMeter.hpp
    include/vts-browser/position.hpp
    include/vts-browser/resources.hpp
    include/vts-browser/search.hpp
    include/vts-browser/view.hpp
    # C API
    include/vts-browser/callbacks.h
    include/vts-browser/camera.h
    include/vts-browser/celestial.h
    include/vts-browser/fetcher.h
    include/vts-browser/foundation.h
    include/vts-browser/internalMemory.h
    include/vts-browser/log.h
    include/vts-browser/map.h
    include/vts-browser/math.h
    include/vts-browser/navigation.h
    include/vts-browser/position.h
    include/vts-browser/resources.h
    include/vts-browser/search.h
)

set(SRC_LIST
    api/boostProgramOptions.cpp
    api/buffer.cpp
    api/exceptions.cpp
    api/log.cpp
    api/mapApiC.cpp
    api/mapApiCpp.cpp
    api/math.cpp
    api/mathColor.cpp
    api/options.cpp
    api/statistics.cpp
    camera/altitude.cpp
    camera/boundLayers.cpp
    camera/camera.cpp
    camera/cameraApi.cpp
    camera/draws.cpp
    camera/preload.cpp
    camera/traversal.cpp
    camera/traverseNode.cpp
    image/image.cpp
    image/image.hpp
    image/jpeg.cpp
    image/png.cpp
    map/atmosphereDensityTexture.cpp
    map/celestialBody.cpp
    map/coordsManip.cpp
    map/credits.cpp
    map/map.cpp
    map/mapLayer.cpp
    map/progress.cpp
    map/search.cpp
    map/surfaceStack.cpp
    navigation/navigation.cpp
    navigation/navigationApi.cpp
    navigation/positionApi.cpp
    navigation/solver.cpp
    navigation/solver.hpp
    resources/auth.cpp
    resources/cache.cpp
    resources/fetcher.cpp
    resources/font.cpp
    resources/geodataProcessing.cpp
    resources/geodataResources.cpp
    resources/manager.cpp
    resources/mapConfig.cpp
    resources/mesh.cpp
    resources/metaTile.cpp
    resources/other.cpp
    resources/resource.cpp
    resources/resources.cpp
    resources/texture.cpp
    utilities/case/lower.hpp
    utilities/case/title.hpp
    utilities/case/upper.hpp
    utilities/array.hpp
    utilities/case.cpp
    utilities/case.hpp
    utilities/dataUrl.cpp
    utilities/dataUrl.hpp
    utilities/detectLanguage.cpp
    utilities/detectLanguage.hpp
    utilities/json.cpp
    utilities/json.hpp
    utilities/obj.cpp
    utilities/obj.hpp
    utilities/threadName.cpp
    utilities/threadName.hpp
    utilities/threadQueue.hpp
    authConfig.hpp
    camera.hpp
    coordsManip.hpp
    credits.hpp
    fetchTask.hpp
    geodata.hpp
    gpuResource.hpp
    hashTileId.hpp
    map.hpp
    mapApiC.hpp
    mapConfig.hpp
    mapLayer.hpp
    metaTile.hpp
    navigation.hpp
    position.hpp
    renderInfos.hpp
    renderTasks.hpp
    resource.hpp
    searchTask.hpp
    tilesetMapping.hpp
    traverseNode.hpp
    validity.hpp

    ${EXTRA_SRC_LIST}
)

set(DATA_LIST
    data/meshes/aabb.obj
    data/meshes/cube.obj
    data/meshes/line.obj
    data/meshes/quad.obj
    data/meshes/rect.obj
    data/meshes/sphere.obj
    data/textures/debugFont2.png
)
if(NOT BUILDSYS_EMBEDDED)
    set(DATA_LIST
        ${DATA_LIST}
        data/textures/helper.jpg
    )
endif()


buildsys_pack_data(initializeBrowserData)
add_library(vts-browser ${VTS_BROWSER_BUILD_LIBRARY} ${SRC_LIST} ${PUB_HDR_LIST} ${DATA_LIST})
target_compile_definitions(vts-browser ${VTS_BROWSER_BUILD_VISIBILITY} VTS_BUILD_${VTS_BROWSER_BUILD_MACRO})
target_compile_definitions(vts-browser PRIVATE ${MODULE_DEFINITIONS})
if(NOT CMAKE_SIZEOF_VOID_P EQUAL 8)
    message(WARNING "Building for 32 bit platform: disabling explicit vectorization EIGEN_DONT_VECTORIZE")
    target_compile_definitions(vts-browser PUBLIC EIGEN_DONT_VECTORIZE=1)
endif()
target_link_libraries(vts-browser ${VTS_BROWSER_BUILD_VISIBILITY} initializeBrowserData ${MODULE_LIBRARIES} ${EXTRA_LIB_DEPS})
target_link_libraries(vts-browser PRIVATE Optick)
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

if(BUILDSYS_EMBEDDED)
    target_compile_definitions(vts-browser PRIVATE VTS_EMBEDDED)
endif()

if(BUILDSYS_UWP)
    foreach(conf IN ITEMS ${CMAKE_CONFIGURATION_TYPES} ${CMAKE_BUILD_TYPE})
        string(TOUPPER ${conf} conf_upper)
        set_target_properties(vts-browser PROPERTIES LINK_FLAGS_${conf_upper} "/NODEFAULTLIB:vccorlib /NODEFAULTLIB:msvcrt vccorlib.lib msvcrt.lib")
    endforeach(conf)
    set_target_properties(vts-browser PROPERTIES LINK_FLAGS_DEBUG "/NODEFAULTLIB:vccorlibd /NODEFAULTLIB:msvcrtd vccorlibd.lib msvcrtd.lib")
endif()



#######################################################################
# Install

if(NOT BUILDSYS_EMBEDDED)

include(GNUInstallDirs)

set(VTSBROWSER_LIBRARIES vts-browser)

# library and binaries
install(TARGETS vts-browser
    COMPONENT browser
    EXPORT vts-browser
    PUBLIC_HEADER DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/vts-browser
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


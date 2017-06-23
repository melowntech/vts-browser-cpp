# Bump shared library version here
set(vts-browser_SO_VERSION 0.0.0)

define_module(LIBRARY vts-browser DEPENDS vts-libs-nucleus http>=1.2 lodepng)

set(PUB_HDR_LIST
	include/vts-browser/buffer.hpp
	include/vts-browser/foundation.hpp
	include/vts-browser/options.hpp
	include/vts-browser/resources.hpp
	include/vts-browser/fetcher.hpp
	include/vts-browser/map.hpp
	include/vts-browser/draws.hpp
	include/vts-browser/statistics.hpp
	include/vts-browser/view.hpp
	include/vts-browser/math.hpp
	include/vts-browser/exceptions.hpp
	include/vts-browser/credits.hpp
	include/vts-browser/search.hpp
)

set(HDR_LIST
	coordsManip.hpp
	image.hpp
	map.hpp
	resources.hpp
	credits.hpp
	obj.hpp
	cache.hpp
	navigationSolver.hpp
)

set(SRC_LIST
	buffer.cpp
	color.cpp
	credits.cpp
	coordsManip.cpp
	exceptions.cpp
	fetcher.cpp
	image.cpp
	mapApi.cpp
	mapUtilities.cpp
	math.cpp
	navigation.cpp
	obj.cpp
	options.cpp
	renderer.cpp
	resourceAuth.cpp
	resourceMapConfig.cpp
	resourceMesh.cpp
	resourceOther.cpp
	resources.cpp
	resourceTexture.cpp
	statistics.cpp
	search.cpp
	cache.cpp
	navigationSolver.cpp
)

include(data.cmake)

buildsys_position_independent(${MODULE_LIBRARIES})

add_library(vts-browser SHARED ${SRC_LIST} ${HDR_LIST} ${PUB_HDR_LIST})
#target_compile_options(vts-browser PRIVATE -Wl,--no-undefined)
buildsys_library(vts-browser)
buildsys_soversion(vts-browser ${vts-browser_SO_VERSION})

# all libraries are PRIVATE -> do not forward static libraries out
target_link_libraries(vts-browser PRIVATE ${MODULE_LIBRARIES})
buildsys_target_compile_definitions(vts-browser ${MODULE_DEFINITIONS})

# enforce the library to have all symbols defined
# does not work at this moment
# target_compile_options(vts-browser PRIVATE -Wl,-z,defs -Bsymbolic)

#######################################################################
# Install

set(INCLUDE_INSTALL_DIR include)
set(LIB_INSTALL_DIR lib)
set(BIN_INSTALL_DIR bin)
set(SHARE_INSTALL_DIR share)
set(VTSBROWSER_LIBRARIES vts-browser)

# headers
install(FILES ${PUB_HDR_LIST}
  DESTINATION ${INCLUDE_INSTALL_DIR}/vts-browser
  COMPONENT browser-dev
  )

# library and binaries
install(TARGETS vts-browser
  EXPORT vts-browser
  RUNTIME DESTINATION ${BIN_INSTALL}
  LIBRARY DESTINATION ${LIB_INSTALL_DIR}
  COMPONENT browser
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

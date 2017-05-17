# Bump shared library version here
set(vts-browser_SO_VERSION 0.0.0)

define_module(LIBRARY vts-browser DEPENDS vts-libs-nucleus http>=1.2 lodepng)

set(PUB_HDR_LIST
	include/vts/buffer.hpp
	include/vts/foundation.hpp
	include/vts/options.hpp
	include/vts/resources.hpp
	include/vts/fetcher.hpp
	include/vts/map.hpp
	include/vts/draws.hpp
	include/vts/statistics.hpp
	include/vts/view.hpp
	include/vts/math.hpp
	include/vts/point.hpp
	include/vts/exceptions.hpp
)

set(HDR_LIST
	map.hpp
	image.hpp
	resourceMap.hpp
	mapConfig.hpp
	csConvertor.hpp
	obj.hpp
	resource.hpp
	auth.hpp
)

set(SRC_LIST
	buffer.cpp
	fetcher.cpp
	options.cpp
	mapResources.cpp
	image.cpp
	math.cpp
	renderer.cpp
	resourceMap.cpp
	color.cpp
	mapUtilities.cpp
	mapConfig.cpp
	navigation.cpp
	resource.cpp
	statistics.cpp
	csConvertor.cpp
	obj.cpp
	resourceGpu.cpp
	mapApi.cpp
	fetchTask.cpp
	exceptions.cpp
	auth.cpp
)

set(DATA_LIST
	data/fonts/roboto-regular.ttf
	data/meshes/aabb.obj
	data/meshes/cube.obj
	data/meshes/line.obj
	data/meshes/sphere.obj
	data/shaders/color.frag.glsl
	data/shaders/color.vert.glsl
	data/shaders/gui.frag.glsl
	data/shaders/gui.vert.glsl
	data/shaders/texture.frag.glsl
	data/shaders/texture.vert.glsl
	data/textures/gwen.png
	data/textures/helper.jpg
)

set(GEN_CODE "\#include <map>\n")
set(GEN_CODE "${GEN_CODE}extern std::map<std::string, std::pair<size_t, const unsigned char*>> data_map\;\n")
set(GEN_CODE "${GEN_CODE}namespace\n{\nclass Data_mapInitializer\n{\npublic:\nData_mapInitializer()\n{\n")
foreach(path ${DATA_LIST})
	string(REPLACE "/" "_" name ${path})
	string(REPLACE "." "_" name ${name})
	string(REPLACE "-" "_" name ${name})
	file_to_cpp("" ${name} ${path})
	list(APPEND HDR_LIST ${CMAKE_CURRENT_BINARY_DIR}/${path}.hpp)
	list(APPEND SRC_LIST ${CMAKE_CURRENT_BINARY_DIR}/${path}.cpp)
	set(GEN_CODE "\#include \"${CMAKE_CURRENT_BINARY_DIR}/${path}.hpp\"\n${GEN_CODE}")
	set(GEN_CODE "${GEN_CODE}data_map[\"${path}\"] = std::make_pair(sizeof(${name}), ${name})\;\n")
endforeach()
set(GEN_CODE "${GEN_CODE}}\n} data_mapInitializer\;\n}\n")
file(WRITE ${CMAKE_CURRENT_BINARY_DIR}/data_map.cpp ${GEN_CODE})
list(APPEND SRC_LIST ${CMAKE_CURRENT_BINARY_DIR}/data_map.cpp)

buildsys_position_independent(${MODULE_LIBRARIES})

add_library(vts-browser SHARED ${SRC_LIST})
buildsys_library(vts-browser)
buildsys_soversion(vts-browser ${vts-browser_SO_VERSION})

# all libraries are PRIVATE -> do not forward static libraries out
target_link_libraries(vts-browser PRIVATE ${MODULE_LIBRARIES})
buildsys_target_compile_definitions(vts-browser ${MODULE_DEFINITIONS})

# trick QtCreator into showing some additional files
add_custom_target(vts-browser-headers SOURCES ${HDR_LIST} ${PUB_HDR_LIST})

#######################################################################
# Install

set(INCLUDE_INSTALL_DIR include)
set(LIB_INSTALL_DIR lib)
set(BIN_INSTALL_DIR bin)
set(SHARE_INSTALL_DIR share)
set(VTSBROWSER_LIBRARIES vts-browser)

# headers
install(FILES ${PUB_HDR_LIST}
  DESTINATION ${INCLUDE_INSTALL_DIR}/vts
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
  COMPONENT browser
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

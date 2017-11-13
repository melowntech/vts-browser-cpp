define_module(LIBRARY vts-renderer DEPENDS vts-browser glad)

set(PUB_HDR_LIST
  include/vts-renderer/foundation.hpp
  include/vts-renderer/renderer.hpp
  include/vts-renderer/classes.hpp
)

set(HDR_LIST
  renderer.hpp
)

set(SRC_LIST
  renderer.cpp
  classes.cpp
  errors.cpp
)

include(data.cmake)

add_library(vts-renderer ${LIB_TYPE} ${SRC_LIST} ${HDR_LIST} ${PUB_HDR_LIST})
target_compile_definitions(vts-renderer ${LIB_VIS} VTSR_BUILD_${LIB_TYPE})
target_link_libraries(vts-renderer ${LIB_VIS} ${MODULE_LIBRARIES})
buildsys_library(vts-renderer)
buildsys_soversion(vts-renderer ${vts-browser_SO_VERSION})
buildsys_target_compile_definitions(vts-renderer ${MODULE_DEFINITIONS})
buildsys_position_independent(${MODULE_LIBRARIES})

if(IOS)
  target_compile_definitions(vts-renderer PRIVATE VTSR_OPENGLES)
else()
  target_compile_definitions(vts-renderer PRIVATE VTSR_INCLUDE_GL)
endif()



#######################################################################
# Install

if(NOT IOS)

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

endif()






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
target_compile_definitions(vts-renderer ${LIB_VIS} VTS_RENDERER_BUILD_${LIB_TYPE})
target_link_libraries(vts-renderer ${LIB_VIS} ${MODULE_LIBRARIES})
buildsys_library(vts-renderer)
buildsys_soversion(vts-renderer ${vts-browser_SO_VERSION})
buildsys_target_compile_definitions(vts-renderer ${MODULE_DEFINITIONS})
buildsys_position_independent(${MODULE_LIBRARIES})

if(IOS)
  target_compile_definitions(vts-renderer PRIVATE VTSR_OPENGLES)
endif()


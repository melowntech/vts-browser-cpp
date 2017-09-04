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

buildsys_position_independent(${MODULE_LIBRARIES})

add_library(vts-renderer SHARED ${SRC_LIST} ${HDR_LIST} ${PUB_HDR_LIST})
target_compile_definitions(vts-browser PRIVATE VTS_RENDERER_BUILD_SHARED)
buildsys_library(vts-renderer)
target_link_libraries(vts-renderer PRIVATE ${MODULE_LIBRARIES})
buildsys_target_compile_definitions(vts-renderer ${MODULE_DEFINITIONS})



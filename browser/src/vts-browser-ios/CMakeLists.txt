
define_module(BINARY vts-browser-ios DEPENDS
  vts-browser vts-renderer
  glad SDL2 THREADS)

set(HDR_LIST

)

set(SRC_LIST
  main.cpp
)

add_executable(vts-browser-ios MACOSX_BUNDLE ${SRC_LIST} ${HDR_LIST})
target_link_libraries(vts-browser-ios ${MODULE_LIBRARIES} objc)
buildsys_binary(vts-browser-ios)
buildsys_target_compile_definitions(vts-browser-ios ${MODULE_DEFINITIONS})
set_target_properties(vts-browser-ios PROPERTIES
  MACOSX_BUNDLE_GUI_IDENTIFIER vts-browser
  MACOSX_BUNDLE_BUNDLE_NAME vts-browser-ios
  MACOSX_BUNDLE_BUNDLE_VERSION ${vts-browser_SO_VERSION}
  MACOSX_BUNDLE_INFO_PLIST ${CMAKE_CURRENT_SOURCE_DIR}/Info.plist.in
)


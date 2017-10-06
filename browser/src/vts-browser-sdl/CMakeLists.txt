
define_module(BINARY vts-browser-sdl DEPENDS
  vts-browser vts-renderer
  glad SDL2 THREADS)

set(SRC_LIST
  main.cpp
)

add_executable(vts-browser-sdl MACOSX_BUNDLE ${SRC_LIST})
target_link_libraries(vts-browser-sdl ${MODULE_LIBRARIES} objc)
buildsys_binary(vts-browser-sdl)
buildsys_target_compile_definitions(vts-browser-sdl ${MODULE_DEFINITIONS})
set_target_properties(vts-browser-sdl PROPERTIES
  MACOSX_BUNDLE_GUI_IDENTIFIER vts-browser
  MACOSX_BUNDLE_BUNDLE_NAME vts-browser-sdl
  MACOSX_BUNDLE_BUNDLE_VERSION ${vts-browser_SO_VERSION}
  MACOSX_BUNDLE_INFO_PLIST ${CMAKE_CURRENT_SOURCE_DIR}/Info.plist.in
  XCODE_ATTRIBUTE_IPHONEOS_DEPLOYMENT_TARGET 10.0
  XCODE_ATTRIBUTE_CLANG_ENABLE_OBJC_ARC YES
  XCODE_ATTRIBUTE_TARGETED_DEVICE_FAMILY "1,2"
)


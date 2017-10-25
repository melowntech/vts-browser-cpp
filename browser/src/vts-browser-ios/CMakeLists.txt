
define_module(BINARY vts-browser-ios DEPENDS
  vts-browser vts-renderer
  glad THREADS)

set(SRC_LIST
  main.m
  AppDelegate.h AppDelegate.mm
  ConfigItem.h ConfigItem.m
  Map.h Map.mm
  TimerObj.h TimerObj.mm
  controllers/UrlsViewController.h controllers/UrlsViewController.m
  controllers/ConfigViewController.h controllers/ConfigViewController.m
  controllers/MapViewController.h controllers/MapViewController.mm
  controllers/SearchViewController.h controllers/SearchViewController.mm
  controllers/SearchCell.h controllers/SearchCell.m
  controllers/detail/PositionViewController.h controllers/detail/PositionViewController.mm
  controllers/detail/OptionsViewController.h controllers/detail/OptionsViewController.mm
  controllers/detail/StatisticsViewController.h controllers/detail/StatisticsViewController.mm
  controllers/detail/AboutViewController.h controllers/detail/AboutViewController.m
)

set(RES_LIST
  Main.storyboard
  Images.xcassets
)

add_executable(vts-browser-ios MACOSX_BUNDLE ${SRC_LIST} ${RES_LIST})
target_link_libraries(vts-browser-ios ${MODULE_LIBRARIES} objc "-framework CoreData" "-framework UIKit" "-framework GLKit" "-framework OpenGLES")
buildsys_binary(vts-browser-ios)
buildsys_target_compile_definitions(vts-browser-ios ${MODULE_DEFINITIONS})
set_target_properties(vts-browser-ios PROPERTIES
  MACOSX_BUNDLE_GUI_IDENTIFIER "VTS Browser"
  MACOSX_BUNDLE_BUNDLE_NAME vts-browser-ios
  MACOSX_BUNDLE_BUNDLE_VERSION ${vts-browser_SO_VERSION}
  MACOSX_BUNDLE_INFO_PLIST ${CMAKE_CURRENT_SOURCE_DIR}/Info.plist.in
  XCODE_ATTRIBUTE_IPHONEOS_DEPLOYMENT_TARGET 10.0
  XCODE_ATTRIBUTE_CLANG_ENABLE_OBJC_ARC YES
  XCODE_ATTRIBUTE_ENABLE_BITCODE NO
  XCODE_ATTRIBUTE_TARGETED_DEVICE_FAMILY "1,2"
  XCODE_ATTRIBUTE_ASSETCATALOG_COMPILER_APPICON_NAME AppIcon
  XCODE_ATTRIBUTE_ASSETCATALOG_COMPILER_LAUNCHIMAGE_NAME LaunchImage
  RESOURCE "${RES_LIST}"
)


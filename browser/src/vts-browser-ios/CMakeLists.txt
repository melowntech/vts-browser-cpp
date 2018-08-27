
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
    Launch.storyboard
    Images.xcassets
)

set(DATA_LIST
    data/textures/border.png
    data/textures/scale-pitch.png
    data/textures/scale-yaw.png
    data/textures/scale-zoom.png
)

buildsys_pack_data(initializeIosData)
add_executable(vts-browser-ios MACOSX_BUNDLE ${SRC_LIST} ${DATA_LIST} ${RES_LIST})
target_link_libraries(vts-browser-ios initializeIosData ${MODULE_LIBRARIES}
    objc "-framework CoreData" "-framework UIKit" "-framework GLKit" "-framework OpenGLES")
target_compile_definitions(vts-browser-ios PRIVATE ${MODULE_DEFINITIONS})
set_target_properties(vts-browser-ios PROPERTIES
    MACOSX_BUNDLE_GUI_IDENTIFIER "VTS Browser"
    MACOSX_BUNDLE_BUNDLE_NAME vts-browser-ios
    MACOSX_BUNDLE_BUNDLE_VERSION ${vts-browser_SO_VERSION}
    MACOSX_BUNDLE_INFO_PLIST ${CMAKE_CURRENT_SOURCE_DIR}/Info.plist.in
    XCODE_ATTRIBUTE_CLANG_ENABLE_OBJC_ARC YES
    XCODE_ATTRIBUTE_ASSETCATALOG_COMPILER_APPICON_NAME AppIcon
    RESOURCE "${RES_LIST}"
    INSTALL_RPATH "@executable_path;@executable_path/Frameworks"
)
buildsys_binary(vts-browser-ios)
buildsys_ide_groups(vts-browser-ios apps)


if(VTS_BROWSER_TYPE STREQUAL SHARED)
    add_custom_command(TARGET vts-browser-ios POST_BUILD
        COMMAND "${CMAKE_COMMAND}" -E copy
            "$<TARGET_FILE:vts-browser>"
            "$<TARGET_FILE_DIR:vts-browser-ios>/$<TARGET_FILE_NAME:vts-browser>"
        COMMAND "${CMAKE_COMMAND}" -E create_symlink
            "$<TARGET_FILE_NAME:vts-browser>"
            "$<TARGET_FILE_DIR:vts-browser-ios>/$<TARGET_SONAME_FILE_NAME:vts-browser>"
        COMMAND "${CMAKE_COMMAND}" -E copy
            "$<TARGET_FILE:vts-renderer>"
            "$<TARGET_FILE_DIR:vts-browser-ios>/$<TARGET_FILE_NAME:vts-renderer>"
        COMMAND "${CMAKE_COMMAND}" -E create_symlink
            "$<TARGET_FILE_NAME:vts-renderer>"
            "$<TARGET_FILE_DIR:vts-browser-ios>/$<TARGET_SONAME_FILE_NAME:vts-renderer>"
        COMMENT "copy vts browser libraries to final bundle"
    )
endif()

if(VTS_BROWSER_TYPE STREQUAL FRAMEWORK)
    add_custom_command(TARGET vts-browser-ios POST_BUILD
        COMMAND "${CMAKE_COMMAND}" -E copy_directory
            "$<TARGET_FILE_DIR:vts-browser>"
            "$<TARGET_FILE_DIR:vts-browser-ios>/Frameworks/$<TARGET_FILE_NAME:vts-browser>.framework"
        COMMAND "${CMAKE_COMMAND}" -E copy_directory
            "$<TARGET_FILE_DIR:vts-renderer>"
            "$<TARGET_FILE_DIR:vts-browser-ios>/Frameworks/$<TARGET_FILE_NAME:vts-renderer>.framework"
        COMMENT "copy vts browser libraries to final bundle"
    )
endif()



add_subdirectory(externals/nuklear)

define_module(BINARY vts-browser-desktop DEPENDS
    vts-browser vts-renderer
    nuklear glad SDL2 THREADS Boost_PROGRAM_OPTIONS)

set(SRC_LIST
    dataThread.cpp dataThread.hpp
    guiSkin.cpp guiSkin.hpp
    gui.cpp
    smoothVariable.hpp
    mainWindow.cpp mainWindow.hpp
    programOptions.cpp programOptions.hpp
    editor.cpp editor.hpp
    main.cpp
    screenshot.cpp
)

set(DATA_LIST
    data/shaders/gui.frag.glsl
    data/shaders/gui.vert.glsl
    data/fonts/Roboto-Regular.ttf
    data/textures/gwen.png
)

buildsys_pack_data(initializeDesktopData)
add_executable(vts-browser-desktop ${SRC_LIST} ${DATA_LIST})
target_link_libraries(vts-browser-desktop initializeDesktopData ${MODULE_LIBRARIES})
target_compile_definitions(vts-browser-desktop PUBLIC ${NUKLEAR_COMPONENTS})
target_compile_definitions(vts-browser-desktop PRIVATE ${MODULE_DEFINITIONS})
buildsys_binary(vts-browser-desktop)
buildsys_ide_groups(vts-browser-desktop apps)



# install
include(GNUInstallDirs)
install(TARGETS vts-browser-desktop
    COMPONENT browser-desktop
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    BUNDLE DESTINATION ${CMAKE_INSTALL_BINDIR}
)






define_module(BINARY vts-browser-qt DEPENDS
    vts-browser vts-renderer THREADS)

set(SRC_LIST
    common.hpp
    main.cpp
    mainWindow.cpp
)

add_executable(vts-browser-qt ${SRC_LIST})
target_link_libraries(vts-browser-qt Qt5::Gui ${MODULE_LIBRARIES})
target_compile_definitions(vts-browser-qt PRIVATE ${MODULE_DEFINITIONS})
buildsys_binary(vts-browser-qt)
buildsys_ide_groups(vts-browser-qt apps)
qt5_use_modules(vts-browser-qt Gui)



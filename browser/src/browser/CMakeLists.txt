
include_directories(include)

add_subdirectory(vts-browser)

if(NOT DEFINED BUILDSYS_NOBUILD_TARGET_vts-browser-qt)
	add_subdirectory(vts-browser-qt)
endif()

if(NOT DEFINED BUILDSYS_NOBUILD_TARGET_vts-browser-glfw)
	add_subdirectory(vts-browser-glfw)
endif()


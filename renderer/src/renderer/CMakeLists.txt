
include_directories(include)

add_subdirectory(renderer-core)

if(NOT DEFINED BUILDSYS_NOBUILD_TARGET_renderer-qt)
	add_subdirectory(renderer-qt)
endif()

if(NOT DEFINED BUILDSYS_NOBUILD_TARGET_renderer-glfw)
	add_subdirectory(renderer-glfw)
endif()


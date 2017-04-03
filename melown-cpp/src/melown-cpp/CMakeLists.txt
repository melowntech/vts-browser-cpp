
include_directories(include)

add_subdirectory(melown-cpp)

if(NOT DEFINED BUILDSYS_NOBUILD_TARGET_melown-qt)
	add_subdirectory(melown-qt)
endif()

if(NOT DEFINED BUILDSYS_NOBUILD_TARGET_melown-glfw)
	add_subdirectory(melown-glfw)
endif()


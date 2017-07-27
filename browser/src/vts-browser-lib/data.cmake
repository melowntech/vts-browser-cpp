
set(DATA_LIST
	data/fonts/roboto-regular.ttf
	data/meshes/aabb.obj
	data/meshes/cube.obj
	data/meshes/line.obj
	data/meshes/quad.obj
	data/meshes/sphere.obj
	data/shaders/atmosphere-back.frag.glsl
	data/shaders/atmosphere-back.vert.glsl
	data/shaders/atmosphere-front.frag.glsl
	data/shaders/atmosphere-front.vert.glsl
	data/shaders/color.frag.glsl
	data/shaders/color.vert.glsl
	data/shaders/gui.frag.glsl
	data/shaders/gui.vert.glsl
	data/shaders/texture.frag.glsl
	data/shaders/texture.vert.glsl
	data/textures/gwen.png
	data/textures/helper.jpg
)

set(GEN_CODE "\#include <map>\n")
set(GEN_CODE "${GEN_CODE}extern std::map<std::string, std::pair<size_t, const unsigned char*>> data_map\;\n")
set(GEN_CODE "${GEN_CODE}namespace\n{\nclass Data_mapInitializer\n{\npublic:\nData_mapInitializer()\n{\n")
foreach(path ${DATA_LIST})
	string(REPLACE "/" "_" name ${path})
	string(REPLACE "." "_" name ${name})
	string(REPLACE "-" "_" name ${name})
	file_to_cpp("" ${name} ${path})
	list(APPEND HDR_LIST ${CMAKE_CURRENT_BINARY_DIR}/${path}.hpp)
	list(APPEND SRC_LIST ${CMAKE_CURRENT_BINARY_DIR}/${path}.cpp)
	set(GEN_CODE "\#include \"${CMAKE_CURRENT_BINARY_DIR}/${path}.hpp\"\n${GEN_CODE}")
	set(GEN_CODE "${GEN_CODE}data_map[\"${path}\"] = std::make_pair(sizeof(${name}), ${name})\;\n")
endforeach()
set(GEN_CODE "${GEN_CODE}}\n} data_mapInitializer\;\n}\n")
file(WRITE ${CMAKE_CURRENT_BINARY_DIR}/data_map.cpp ${GEN_CODE})
list(APPEND SRC_LIST ${CMAKE_CURRENT_BINARY_DIR}/data_map.cpp)

add_custom_target(data SOURCES ${DATA_LIST})

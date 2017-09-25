
set(DATA_LIST
	data/fonts/roboto-regular.ttf
	data/meshes/aabb.obj
	data/meshes/cube.obj
	data/meshes/line.obj
	data/meshes/quad.obj
	data/meshes/sphere.obj
	data/shaders/atmosphere.frag.glsl
	data/shaders/atmosphere.vert.glsl
	data/shaders/surface.frag.glsl
	data/shaders/surface.vert.glsl
	data/shaders/infographic.frag.glsl
	data/shaders/infographic.vert.glsl
	data/shaders/gui.frag.glsl
	data/shaders/gui.vert.glsl
	data/textures/gwen.png
	data/textures/helper.jpg
)

set(GEN_CODE)
set(GEN_CODE "${GEN_CODE}\#include <string>\n")
set(GEN_CODE "${GEN_CODE}namespace vts { namespace detail { void addInternalMemoryData(const std::string name, const unsigned char *data, size_t size)\; } }\n")
set(GEN_CODE "${GEN_CODE}using namespace vts::detail\;\n")
set(GEN_CODE "${GEN_CODE}namespace vts { namespace renderer { namespace priv { \nvoid initializeRenderData()\n{\n")
foreach(path ${DATA_LIST})
	string(REPLACE "/" "_" name ${path})
	string(REPLACE "." "_" name ${name})
	string(REPLACE "-" "_" name ${name})
	file_to_cpp("" ${name} ${path})
	list(APPEND HDR_LIST ${CMAKE_CURRENT_BINARY_DIR}/${path}.hpp)
	list(APPEND SRC_LIST ${CMAKE_CURRENT_BINARY_DIR}/${path}.cpp)
	set(GEN_CODE "\#include \"${CMAKE_CURRENT_BINARY_DIR}/${path}.hpp\"\n${GEN_CODE}")
	set(GEN_CODE "${GEN_CODE}addInternalMemoryData(\"${path}\", ${name}, sizeof(${name}))\;\n")
endforeach()
set(GEN_CODE "${GEN_CODE}}\n} } }\n")
file(WRITE ${CMAKE_CURRENT_BINARY_DIR}/data_map.cpp ${GEN_CODE})
list(APPEND SRC_LIST ${CMAKE_CURRENT_BINARY_DIR}/data_map.cpp)

add_custom_target(vts-renderer-data SOURCES ${DATA_LIST})


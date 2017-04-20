define_module(LIBRARY vts-browser DEPENDS vts-libs-core http lodepng)

set(PUB_HDR_LIST
	../include/vts/buffer.hpp
	../include/vts/foundation.hpp
	../include/vts/options.hpp
	../include/vts/resources.hpp
	../include/vts/fetcher.hpp
	../include/vts/map.hpp
	../include/vts/rendering.hpp
	../include/vts/statistics.hpp
	../include/vts/view.hpp
	../include/vts/math.hpp
	../include/vts/point.hpp
)

set(HDR_LIST
	map.hpp
	image.hpp
	resourceMap.hpp
	mapConfig.hpp
	csConvertor.hpp
	obj.hpp
	resource.hpp
)

set(SRC_LIST
	buffer.cpp
	fetcher.cpp
	options.cpp
	resourceManager.cpp
	image.cpp
	math.cpp
	renderer.cpp
	resourceMap.cpp
	color.cpp
	rendererUtilities.cpp
	mapConfig.cpp
	navigation.cpp
	resource.cpp
	statistics.cpp
	csConvertor.cpp
	obj.cpp
	resourceGpu.cpp
	mapFoundation.cpp
)

set(DATA_LIST
	data/cube.obj
	data/helper.jpg
	data/fonts/roboto-regular.ttf
	data/shaders/color.frag.glsl
	data/shaders/color.vert.glsl
	data/shaders/gui.frag.glsl
	data/shaders/gui.vert.glsl
	data/shaders/tex.frag.glsl
	data/shaders/tex.vert.glsl
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

buildsys_position_independent(${MODULE_LIBRARIES})

add_library(vts-browser SHARED ${SRC_LIST})
buildsys_library(vts-browser)
buildsys_soversion(vts-browser 0.0.0)
target_link_libraries(vts-browser ${MODULE_LIBRARIES})
buildsys_target_compile_definitions(vts-browser ${MODULE_DEFINITIONS})

# trick QtCreator into showing some additional files
add_custom_target(vts-browser-headers SOURCES ${HDR_LIST} ${PUB_HDR_LIST})

# install
install(TARGETS vts-browser
			RUNTIME DESTINATION bin
			LIBRARY DESTINATION lib
			COMPONENT browser
)

install(FILES ${PUB_HDR_LIST}
			DESTINATION include/vts
			COMPONENT browser-dev
)



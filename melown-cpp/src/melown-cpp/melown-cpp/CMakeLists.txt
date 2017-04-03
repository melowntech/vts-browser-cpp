define_module(LIBRARY melown-cpp DEPENDS vts-libs-core http lodepng)

set(PUB_HDR_LIST
	../include/melown/buffer.hpp
	../include/melown/foundation.hpp
	../include/melown/options.hpp
	../include/melown/resources.hpp
	../include/melown/fetcher.hpp
	../include/melown/map.hpp
	../include/melown/rendering.hpp
	../include/melown/statistics.hpp
)

set(HDR_LIST
	map.hpp
	image.hpp
	math.hpp
	resourceMap.hpp
	color.hpp
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

buildsys_position_independent(${MODULE_LIBRARIES})

add_library(melown-cpp SHARED ${SRC_LIST})
buildsys_library(melown-cpp)
target_link_libraries(melown-cpp ${MODULE_LIBRARIES})
buildsys_target_compile_definitions(melown-cpp ${MODULE_DEFINITIONS})

# trick QtCreator into showing some additional files
add_custom_target(melownCppHeaders SOURCES ${HDR_LIST} ${PUB_HDR_LIST})



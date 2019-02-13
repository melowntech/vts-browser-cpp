
file(GLOB_RECURSE sheenbidi-sources "SheenBidi/Headers/*" "SheenBidi/Source/*")

if(UNIX)
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -w")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -w")
endif()

add_library(SheenBidi STATIC ${sheenbidi-sources})
target_include_directories(SheenBidi SYSTEM PUBLIC SheenBidi/Headers)
buildsys_ide_groups(SheenBidi deps)

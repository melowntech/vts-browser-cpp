
file(GLOB_RECURSE sheenbidi-sources "SheenBidi/Headers/*" "SheenBidi/Source/*")

add_library(SheenBidi STATIC ${sheenbidi-sources})
target_include_directories(SheenBidi PUBLIC SheenBidi/Headers)
buildsys_ide_groups(SheenBidi deps)

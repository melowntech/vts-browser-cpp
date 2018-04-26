
project(vts-browser-minimal-cs CSharp)

set(SRC_LIST
    Program.cs
)

add_executable(vts-browser-minimal-cs ${SRC_LIST})
target_link_libraries(vts-browser-minimal-cs vts-browser-cs vts-renderer-cs)
buildsys_ide_groups(vts-browser-minimal-cs apps)
set_target_properties(vts-browser-minimal-cs PROPERTIES VS_GLOBAL_ROOTNAMESPACE "vtsBrowserMinimalCs")


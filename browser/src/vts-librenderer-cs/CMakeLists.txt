
project(vts-renderer-cs CSharp)

set(SRC_LIST
    Interop.cs
    Renderer.cs
    RenderOptions.cs
    RenderVariables.cs
)

add_library(vts-renderer-cs ${SRC_LIST})
target_link_libraries(vts-renderer-cs vts-browser-cs)
add_dependencies(vts-renderer-cs vts-renderer)
buildsys_ide_groups(vts-renderer-cs libs)
set_target_properties(vts-renderer-cs PROPERTIES VS_GLOBAL_ROOTNAMESPACE "vts")


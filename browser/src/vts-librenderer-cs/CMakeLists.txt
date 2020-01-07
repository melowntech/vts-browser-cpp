
project(vts-renderer-cs CSharp)

set(SRC_LIST
    Interop.cs
    RenderContext.cs
    RenderOptions.cs
    RenderVariables.cs
	RenderView.cs
)

add_library(vts-renderer-cs SHARED ${SRC_LIST})
add_dependencies(vts-renderer-cs vts-renderer)
target_link_libraries(vts-renderer-cs vts-browser-cs)
set_target_properties(vts-renderer-cs PROPERTIES VS_GLOBAL_ROOTNAMESPACE "vts")
buildsys_ide_groups(vts-renderer-cs libs)


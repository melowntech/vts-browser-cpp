
project(vts-browser-cs CSharp)

set(SRC_LIST
    Callbacks.cs
    Draws.cs
    Exception.cs
    Interop.cs
    Map.cs
    Math.cs
    Resources.cs
    Utilities.cs
    Searching.cs
)

add_library(vts-browser-cs ${SRC_LIST})
add_dependencies(vts-browser-cs vts-browser)
set_target_properties(vts-browser-cs PROPERTIES VS_GLOBAL_ROOTNAMESPACE "vts")
set_target_properties(vts-browser-cs PROPERTIES VS_DOTNET_REFERENCES "System")
buildsys_ide_groups(vts-browser-cs libs)


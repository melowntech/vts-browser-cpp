
project(vts-browser-cs CSharp)

set(SRC_LIST
    Callbacks.cs
    CameraBase.cs
    DrawBase.cs
    Exception.cs
    Interop.cs
    Map.cs
    Resources.cs
    Utilities.cs
)

add_library(vts-browser-cs ${SRC_LIST})
add_dependencies(vts-browser-cs vts-browser)
buildsys_ide_groups(vts-browser-cs libs)
set_target_properties(vts-browser-cs PROPERTIES VS_GLOBAL_ROOTNAMESPACE "vts")
set_target_properties(vts-browser-cs PROPERTIES VS_DOTNET_REFERENCES "System")



project(vts-browser-cs CSharp)

set(SRC_LIST
    CameraBase.cs
    DrawBase.cs
    Interop.cs
    Map.cs
    Exception.cs
    Utilities.cs
)

add_library(vts-browser-cs ${SRC_LIST})
add_dependencies(vts-browser-cs vts-browser)
buildsys_ide_groups(vts-browser-cs libs)
set_target_properties(vts-browser-cs PROPERTIES VS_GLOBAL_ROOTNAMESPACE "vts")


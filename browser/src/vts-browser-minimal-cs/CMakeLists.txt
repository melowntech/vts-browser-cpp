
project(vts-browser-minimal-cs CSharp)

set(SRC_LIST
    VtsGLControl.cs
    VtsGLControl.Designer.cs
    Window.cs
    Window.Designer.cs
)

add_executable(vts-browser-minimal-cs ${SRC_LIST})
target_link_libraries(vts-browser-minimal-cs vts-browser-cs vts-renderer-cs)
set_target_properties(vts-browser-minimal-cs PROPERTIES VS_GLOBAL_ROOTNAMESPACE "vtsBrowserMinimalCs")
set_target_properties(vts-browser-minimal-cs PROPERTIES VS_DOTNET_REFERENCES "System;System.Drawing;System.Windows.Forms")
buildsys_ide_groups(vts-browser-minimal-cs apps)

include(CSharpUtilities)
csharp_set_windows_forms_properties(${SRC_LIST})

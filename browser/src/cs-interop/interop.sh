#!/bin/bash

cat ../vts-libbrowser/include/vts-browser/*.h | ./c2cs.sh vts-browser | sed 's/CLASSNAME/BrowserInterop/g' > ../vts-libbrowser-cs/Interop.cs

cat ../vts-librenderer/include/vts-renderer/*.h | sed 's/VTSR_API/VTS_API/g' - | ./c2cs.sh vts-renderer | sed 's/CLASSNAME/RendererInterop/g' > ../vts-librenderer-cs/Interop.cs

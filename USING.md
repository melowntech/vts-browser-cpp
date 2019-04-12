
# Using the Libraries

Install the libraries from apt packages or [build](BUILDING.md) them.

Write a cmake file _CMakeLists.txt_:

```cmake
cmake_minimum_required(VERSION 3.0)
project(vts-example CXX)
set(CMAKE_CXX_STANDARD 11)

find_package(VtsBrowser REQUIRED)
include_directories(${VtsBrowser_INCLUDE_DIR})
find_package(VtsRenderer REQUIRED)
include_directories(${VtsRenderer_INCLUDE_DIR})
find_package(SDL2 REQUIRED)
include_directories(${SDL2_INCLUDE_DIR})

add_executable(vts-example main.cpp)
target_link_libraries(vts-example ${VtsBrowser_LIBRARIES} ${VtsRenderer_LIBRARIES} SDL2)
```

Copy the _main.cpp_ from the [vts-browser-minimal](https://github.com/melowntech/vts-browser-cpp/wiki/examples-minimal).

Build and run:

```sh
mkdir build && cd build
cmake -DCMAKE_BUILD_TYPE=RELWITHDEBINFO ..
cmake --build .
./vts-example
```


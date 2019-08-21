
# Build for Windows Desktop

You have multiple options:

 - Download (or build) and install all required dependencies yourself.
   Then use cmake.
 - (Recommended) Use [Build Wrapper](https://github.com/melowntech/vts-browser-cpp-build-wrapper) instead.
   The wrapper has integrated build scripts for all dependencies making building the browser a peace of cake :D

# Build for UWP (Universal Windows Platform)

You have multiple options:

 - Build all required dependencies yourself.
   Then use cmake with -DCMAKE_TOOLCHAIN_FILE=\<path-to-our-uwp.toolchain.cmake\>.
 - (Recommended) Use [Build Wrapper](https://github.com/melowntech/vts-browser-cpp-build-wrapper) instead.
   The wrapper has integrated build scripts for all dependencies making building the browser a peace of cake :D

# Build for Linux Desktop

The Gdal library must be version 2. Additional source list is required on eg. Ubuntu Xenial.

```bash
sudo add-apt-repository ppa:ubuntugis/ppa
```

Install packages that are required to build the library.

```bash
sudo apt update
sudo apt install \
    cmake \
    python-minimal \
    libboost-all-dev \
    libeigen3-dev \
    libgdal-dev \
    libproj-dev \
    libgeographic-dev \
    libjsoncpp-dev \
    libsdl2-dev \
    libfreetype6-dev \
    libharfbuzz-dev
```

Clone the git repository with all submodules.
The library build system is cmake based.
However, for convenience, a make shortcut exists.

```bash
git clone --recursive https://github.com/melowntech/vts-browser-cpp.git
cd vts-browser-cpp/browser
make
```

And run the example application.

```bash
bin/vts-browser-desktop
```

# Build for macOS

Install all required libraries using eg. brew or ports.

```bash
sudo port install \
    cmake \
    boost \
    gdal \
    geographiclib \
    jsoncpp \
    eigen3 \
    libsdl2
```

Clone the git repository with all submodules.
Use cmake to generate xcode project.

```bash
git clone --recursive https://github.com/melowntech/vts-browser-cpp.git
cd vts-browser-cpp/browser
mkdir build-osx
cd build-osx
cmake -GXcode ..
```

Use the generated xcode project as usual.

# Build for iOS

You have multiple options:

 - Build all required dependencies yourself.
   Then use cmake with -DCMAKE_TOOLCHAIN_FILE=\<path-to-our-ios.toolchain.cmake\>.
 - (Recommended) Use [Build Wrapper](https://github.com/melowntech/vts-browser-cpp-build-wrapper) instead.
   The wrapper has integrated build scripts for all dependencies making building the browser a peace of cake :D

# Recommended Folder Structure

```
├── vts-browser-cpp
│   ├── browser
│   │   ├── build
│   │   │   ├── vts-browser-cpp.sln -- here is your project for Windows
│   │   │   ├── Makefile -- here is your makefile for Linux
│   │   │   └── vts-browser.xcodeproj -- here is your project for Mac
│   │   ├── build-uwp
│   │   │   ├── vts-browser-cpp.sln -- here is your solution for UWP
│   │   ├── build-ios
│   │   │   └── vts-browser.xcodeproj -- here is your project for iOS
│   │   ├── cmake
│   │   │   ├── ios.toolchain.cmake -- this is where CMAKE_TOOLCHAIN_FILE points to when building for iOS
│   │   │   └── uwp.toolchain.cmake -- this is where CMAKE_TOOLCHAIN_FILE points to when building for UWP
│   │   └── CMakeLists.txt
│   ├── externals
│   │   └── vts-libs -- if this folder is empty, you forgot to clone the submodules (run 'git submodule update --init --recursive' to initialize the submodules now)
│   │       ├── LICENSE
│   │       └── README.md
│   ├── BUILDING.md
│   ├── LICENSE
│   └── README.md
└── your-new-awesome-application-using-vts
    └── CMakeLists.txt
```





# Build for Windows Desktop

You have multiple options:

 - Download (or build) and install all required dependencies yourself.
 - (Recommended) Use [Build Wrapper](https://github.com/Melown/vts-browser-cpp-build-wrapper) instead.
   The wrapper has integrated build scripts for all dependencies making building the browser a peace of cake :D

# Build for Linux Desktop

Install packages that are required to build the library.

```bash
sudo apt install \
    libboost-all-dev \
    libeigen3-dev \
    libgdal-dev \
    libproj-dev \
    libgeographic-dev \
    libjsoncpp-dev \
    libsdl2-dev
```

Clone the git repository with all submodules.
The library build system is cmake based.
However, for a convenience, a make shortcut exists.

```bash
git clone --recursive https://github.com/Melown/vts-browser-cpp.git
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
git clone --recursive https://github.com/Melown/vts-browser-cpp.git
cd vts-browser-cpp/browser
mkdir build-osx
cd build-osx
cmake -GXcode ..
```

Use the generated xcode project as usual.

# Build for iOS

You have multiple options:

 - Build all required dependencies yourself.
 - (Recommended) Use [Build Wrapper](https://github.com/Melown/vts-browser-cpp-build-wrapper) instead.
   The wrapper has integrated build scripts for all dependencies making building the browser a peace of cake :D

# Recommended Folder Structure

```
├── vts-browser-cpp
│   ├── browser
│   │   ├── build
│   │   │   ├── vts-browser-cpp.sln -- here is your solution for Visual Studio
│   │   │   ├── Makefile -- here is your makefile for Linux
│   │   │   └── vts-browser.xcodeproj -- here is your project for Mac
│   │   ├── build-ios
│   │   │   └── vts-browser.xcodeproj -- here is your project for iOS
│   │   ├── cmake
│   │   │   └── ios.toolchain.cmake -- this is where CMAKE_TOOLCHAIN_FILE points to when building for iOS
│   │   └── CMakeLists.txt
│   ├── externals
│   │   └── vts-libs -- if this folder is empty, you forgot to clone the submodules (run 'git submodule update --init --recursive' to initialize the submodules now)
│   │       ├── LICENSE
│   │       └── README.md
│   ├── BUILDING.md
│   ├── LICENSE
│   ├── README.md
│   └── .git
└── your-new-awesome-application-using-vts
    └── CMakeLists.txt or xcodeproj which links to the libraries
```




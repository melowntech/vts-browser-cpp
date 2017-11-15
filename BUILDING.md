
## Build for Linux Desktop

Install required packages to build the library.

```bash
sudo apt-get install \
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
bin/vts-browser-desktop <mapconfig-url>
```

## Build for Mac OS X

Install all required libraries using eg. brew or ports.

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

## Build for iOS

Build all required library dependencies and prepare cmake find package modules for them.

Clone the git repository with all submodules.
Use cmake to generate xcode project.

```bash
git clone --recursive https://github.com/Melown/vts-browser-cpp.git
cd vts-browser-cpp/browser
mkdir build-ios
cd build-ios
cmake -DCMAKE_MODULE_PATH="path to the prepared cmake modules directory" -DCMAKE_TOOLCHAIN_FILE=../toolchains/ios.cmake -GXcode ..
```

Use the generated xcode project as usual.





# Build for Linux desktop

These instructions apply when building vts browser with system dependencies.
To build VTS with embedded dependencies, use the
[Build Wrapper](https://github.com/melowntech/vts-browser-cpp-build-wrapper) instead.

Install packages that are required to build the library.

```bash
sudo apt update
sudo apt install \
    cmake \
    nasm \
    libssl-dev \
    python-minimal \
    libboost-all-dev \
    libeigen3-dev \
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

# Build for other platforms

Use [Build Wrapper](https://github.com/melowntech/vts-browser-cpp-build-wrapper) instead.
The wrapper has integrated build scripts for all dependencies making building the browser a peace of cake :D

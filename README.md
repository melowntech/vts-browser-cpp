# vts-browser-cpp

[vts-browser-cpp](https://github.com/melown/vts-browser-cpp) is a library
that brings vts map capabilities to your native c++ applications.

## User documentation

vts-browser-cpp user documentation is available at the
[wiki](https://github.com/Melown/vts-browser-cpp/wiki)

## Install from Melown repository

We are currently working on it.

## Build from source

Clone the git repository with all submodules.
The library build system is cmake based.
However, for a convenience, a make shortcuts exists.

```
git clone --recursive https://github.com/Melown/vts-browser-cpp.git
cd vts-browser-cpp/browser
make
```

### Dependencies

Packages needed to build the library.

```
sudo apt-get install \
    libboost-all-dev \
	libeigen3-dev \
	libgdal-dev \
    libproj-dev \
	libgeographic-dev \
	libjsoncpp-dev
```

Additional packages needed to build the example applications.

```
sudo apt-get install \
	libglfw3-dev \
	qt5-default
```

## License

See the [LICENSE](LICENSE) file for vts-browser-cpp license.

## How to contribute

Check the [CONTRIBUTING.md](CONTRIBUTING.md) file.


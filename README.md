# vts-browser-cpp

[vts-browser-cpp](https://github.com/melown/vts-browser-cpp) is a library
that brings vts map capabilities to your native c++ applications.

## Design & Features

- simple -> minimalistic Qt based application using this library has about 800 LOC.
- clean c++11 classfull api
- rendering api independent -> the library, on its own, does not render anything, it just tells the application what to render

## WIP

Be warned, this library is still in early state of development.
We make no attempt on maintaining ABI nor API compatibility at this moment.

## Api Documentation

vts-browser-cpp api documentation is available at the
[wiki](https://github.com/Melown/vts-browser-cpp/wiki).

Documentation for the whole vts maps technology is at
[read the docs](https://melown.readthedocs.io).

## Install from Melown Repository

We provide precompiled packages for some popular linux distributions.
See [Melown OSS package repository](https://cdn.melown.com/packages/) for more information.

## Build from Source

Install required packages to build the library.

```bash
sudo apt-get install \
	libboost-all-dev \
	libeigen3-dev \
	libgdal-dev \
	libproj-dev \
	libgeographic-dev \
	libjsoncpp-dev
```

Install optional packages required to build the example applications.

```bash
sudo apt-get install \
	libglfw3-dev \
	qt5-default
```

Clone the git repository with all submodules.
The library build system is cmake based.
However, for a convenience, a make shortcuts exist.

```bash
git clone --recursive https://github.com/Melown/vts-browser-cpp.git
cd vts-browser-cpp/browser
make
```

And run the example application.

```bash
bin/vts-browser-glfw <mapconfig-url>
```

## Bug Reports

The vts-browser-cpp [issue tracker](https://github.com/melown/vts-browser-cpp/issues) is the
place to report bugs or request enhancements. To submit a bug be sure to specify
the vts-browser-cpp version you are using, the appropriate component and a description of how
to reproduce the bug.

## How to Contribute

Check the [CONTRIBUTING.md](CONTRIBUTING.md) file.

## License

See the [LICENSE](LICENSE) file for vts-browser-cpp license.


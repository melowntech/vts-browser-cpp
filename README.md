# VTS Browser CPP

[VTS Browser CPP](https://github.com/melown/vts-browser-cpp) is a library
that brings VTS capabilities to your native c++ applications.

## Design & Features

- Simple -> minimal application using this library has about 150 LOC.
  See [vts-browser-minimal](https://github.com/Melown/vts-browser-cpp/wiki/examples-minimal).
- Clean c++11 class-full API.
- Highly flexible -> almost all aspects can be changed through configuration or callbacks.
- Rendering API independent -> The library, on its own, does not render anything.
  Instead, it just tells the application what to render.
  However, a convenient OpenGL-based rendering library is also provided.

### WIP

Be warned, this library is still in development.
We make no attempt on maintaining ABI nor API compatibility yet.

## Documentation

Browser documentation is available at the
[wiki](https://github.com/melown/vts-browser-cpp/wiki).

Documentation for the whole VTS is at
[Read the Docs](https://melown.readthedocs.io).

### Install from Melown Repository (Linux Desktop)

We provide pre-compiled packages for some popular linux distributions.
See [Melown OSS package repository](https://cdn.melown.com/packages/) for more information.

The packages are named _libvts-browser0_ (the library itself),
_libvts-browser-dbg_ (debug symbols for the library),
_libvts-browser-dev_ (developer files for the library)
and _vts-browser-desktop_ (example application).

### Build from Source

See [BUILDING.md](BUILDING.md) for detailed instructions.

## Bug Reports

For bug reports or enhancement suggestions use the
[Issue tracker](https://github.com/melown/vts-browser-cpp/issues).

## How to Contribute

Check the [CONTRIBUTING.md](CONTRIBUTING.md) file.

## License

See the [LICENSE](LICENSE) file.




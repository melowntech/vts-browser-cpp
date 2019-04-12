# VTS Browser CPP

[VTS Browser CPP](https://github.com/melowntech/vts-browser-cpp) is a collection of libraries
that bring VTS client capabilities to your native applications.

## Examples

### Unity 3D Integration Example

[![Youtube Preview](https://raw.githubusercontent.com/melowntech/vts-browser-unity-plugin/master/screenshots/alps-aircraft.png)](https://www.youtube.com/watch?v=FuVA15Cj54k&feature=youtu.be)

(Click the image to play)

### iOS App Example

[![Youtube Preview](https://raw.githubusercontent.com/wiki/melowntech/vts-browser-cpp/vts-browser-ios.jpg)](https://www.youtube.com/watch?v=BP_zyMTHVlg&feature=youtu.be)

(Click the image to play)

## Features

- Highly flexible -> almost all aspects can be changed through configuration.
- Rendering API independent -> the browser library, on its own, does not render anything.
  Instead, it just tells the application what to render.
  - Additional OpenGL (ES) rendering library is also provided.
- Clean C++ API.
  - C and C# bindings are available too.
- Works on Windows, Linux, macOS and iOS.
  - We also provide [plugin for VTS integration into Unity 3D](https://github.com/melowntech/vts-browser-unity-plugin).
  - And [javascript browser](https://github.com/melowntech/vts-browser-js) for integration with websites.
- Simple -> minimal application using these libraries has about 200 LOC.
  See [vts-browser-minimal](https://github.com/melowntech/vts-browser-cpp/wiki/examples-minimal).

### WIP

Be warned, this library is still in development.
We make no attempt on maintaining ABI nor API compatibility yet.

## Documentation

Browser documentation is available at the
[wiki](https://github.com/melowntech/vts-browser-cpp/wiki).

Documentation for the whole VTS is at
[Read the Docs](https://melown.readthedocs.io).

## Obtaining the VTS Browser Libraries

### Install from Melown Repository (Linux Desktop)

We provide pre-compiled packages for some popular linux distributions.
See [Melown OSS package repository](https://cdn.melown.com/packages/) for more information.

The packages are named _libvts-browser0_ (the library itself),
_libvts-browser-dbg_ (debug symbols for the library),
_libvts-browser-dev_ (developer files for the library)
and _vts-browser-desktop_ (example application).

### Building From Source

See [BUILDING.md](BUILDING.md) for detailed instructions.

## Running Example Application

Run the desktop example application with default mapconfig (our Intergeo presentation):
```bash
vts-browser-desktop
```

Run the desktop example application with specific mapconfig:
```bash
vts-browser-desktop https://cdn.melown.com/mario/store/melown2015/map-config/melown/Melown-Earth-Intergeo-2017/mapConfig.json
```

## Development Using the Libraries

See [USING.md](USING.md) for detailed instructions.

## Bug Reports

For bug reports or enhancement suggestions use the
[Issue tracker](https://github.com/melowntech/vts-browser-cpp/issues).

## How to Contribute

Check the [CONTRIBUTING.md](CONTRIBUTING.md) file.

## License

See the [LICENSE](LICENSE) file.





<img width="390" alt="VTS Browser JS" src="https://github.com/melowntech/assets/blob/master/vts-browser-cpp/vts-browser-cpp-no-left-margin.png?raw=true">

[VTS Browser CPP](https://github.com/melowntech/vts-browser-cpp)
is a collection of libraries that bring VTS frontend capabilities to your native applications.

## Examples

### Unity 3D integration example

[![Youtube Preview](https://raw.githubusercontent.com/melowntech/vts-browser-unity-plugin/master/screenshots/alps-aircraft.png)](https://www.youtube.com/watch?v=FuVA15Cj54k&feature=youtu.be)

(Click to play the video)

### iOS app example

[![Youtube Preview](https://raw.githubusercontent.com/wiki/melowntech/vts-browser-cpp/vts-browser-ios.jpg)](https://www.youtube.com/watch?v=BP_zyMTHVlg&feature=youtu.be)

(Click to play the video)

## Features

- Highly flexible -> almost all aspects can be changed through configuration.
- Rendering API independent -> the browser library, on its own, does not render anything.
  Instead, it just tells the application what to render.
  - Optional OpenGL (ES) rendering library is also provided.
- Clean C++ API.
  - C and C# bindings are available too.
- Works on Windows, UWP (experimental), Linux, Web Assembly (experimental), macOS and iOS.
  - We also provide [plugin for VTS integration into Unity 3D](https://github.com/melowntech/vts-browser-unity-plugin).
  - And [javascript browser](https://github.com/melowntech/vts-browser-js) for integration with websites.
- Simple -> minimal application using these libraries has about 300 LOC.
  See [vts-browser-minimal](https://github.com/melowntech/vts-browser-cpp/wiki/examples-minimal).

### WIP

Be warned, this library is still in development.
We make no attempt on maintaining ABI nor API compatibility yet.

## Documentation

Browser documentation is available at the
[wiki](https://github.com/melowntech/vts-browser-cpp/wiki).

Documentation for the whole VTS is at
[VTS Geospatial](https://vts-geospatial.org).

## Installing from Melown repository (Linux desktop only)

We provide pre-compiled packages for some popular linux distributions.
See [Melown OSS package repository](https://cdn.melown.com/packages/) for more information.

The packages are named _libvts-browser0_ (the library itself),
_libvts-browser-dbg_ (debug symbols for the library),
_libvts-browser-dev_ (developer files for the library)
and _vts-browser-desktop_ (example application).

## Building and using the browser

See [BUILDING.md](BUILDING.md) for instructions to build the libraries from source.

See [USING.md](USING.md) for instructions to write a simple app with VTS browser.

## Running example application

Run the desktop example application with default mapconfig (our Intergeo presentation):
```bash
vts-browser-desktop
```

Run the desktop example application with specific mapconfig:
```bash
vts-browser-desktop https://cdn.melown.com/mario/store/melown2015/map-config/melown/Melown-Earth-Intergeo-2017/mapConfig.json
```

## Bug reports

For bug reports or enhancement suggestions use the
[Issue tracker](https://github.com/melowntech/vts-browser-cpp/issues).

## How to contribute

Check the [CONTRIBUTING.md](CONTRIBUTING.md) file.

## License

See the [LICENSE](LICENSE) file.





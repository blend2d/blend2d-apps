Blend2D Samples
---------------

This repository contains a collection of samples that use [Blend2D](https://blend2d.com) rendering engine.

The following cateogires of samples exist:

  * `getting-started` - The same samples as provided by the [Getting Started](https://blend2d.com/getting-started.html) page
  * `qt` - Interactive samples that use Qt toolkit for UI and quality/performance comparison

Building
--------

Use the following commands to fetch `asmjit`, `blend2d`, and `blend2d-samples` package:

```bash
# Download source packages from Git.
$ git clone https://github.com/asmjit/asmjit
$ git clone https://github.com/blend2d/blend2d
$ git clone https://github.com/blend2d/blend2d-samples
```

Each samples category is placed in its own subdirectory that contains an independent `CMakeLists.txt`, so use the following to build the samples you are interested in:

```bash
$ cd blend2d-samples/<category>
$ mkdir build
$ cd build
$ cmake .. -DCMAKE_BUILD_TYPE=Release
```

Alternatively you can pick a configure script from `<category>/tools` directory if there is a suitable one for your configuration. Please note that samples were divided into categories so people can build only samples they are interested in. If you want to compile and run Qt samples you would need Qt library installed. CMake should be able to find Qt automatically if it's in your `PATH`, alternatively set `CMAKE_PREFIX_PATH` to tell CMake where Qt is installed.

Resources
---------

Some applications require files that are stored in `resources` directory to work properly. Copying all resources to the build directory should be handled by the build script, but if it fails for whatever reason just copy the content of `resources` to your build directory. If these files are missing the sample should report an error.

All files within resources should be freely redistributable:

  - `resources/texture.jpeg `(Public Domain) - downloaded from [publicdomainpictures.net](https://www.publicdomainpictures.net/en/view-image.php?image=9670&picture=colorful-autumn-leaves)
  - `resources/NotoSans-Regular.ttf` (OFL Version 1.1) - downloaded from [Google WebFonts Repo](https://github.com/google/fonts/)
  - `qt/src/bl-qt-tiger.h` uses a tiger data extracted from [amanithvg](http://www.amanithvg.com/)'s tiger example so this data is not covered by the license

Preview
-------

![Demo App Preview](https://blend2d.com/resources/images/demo-app-1.png) | ![Demo App Preview](https://blend2d.com/resources/images/demo-app-3.png)
---- | ----
![Demo App Preview](https://blend2d.com/resources/images/demo-app-4.png) | ![Demo App Preview](https://blend2d.com/resources/images/demo-app-6.png)

License
-------

All code samples can be distributed under either Public Domain (UNLICENSE) or Zlib license.

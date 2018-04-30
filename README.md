Blend2D - Getting Started and Sample Applications
-------------------------------------------------

This repository contains few samples that use Blend2D rendering engine. Many samples come from [Getting Started](https://blend2d.com/getting-started.html) page. It's expected (by default) that the directory structure is similar to the directory structure described in the Getting Started page. Use the following commands to fetch asmjit, blend2d, and these samples:

```base
$ git clone --depth=1 https://github.com/asmjit/asmjit --branch:next-wip
$ git clone --depth=1 https://github.com/blend2d/b2d
$ git clone --depth=1 https://github.com/blend2d/b2d-samples
```

After you have these 3 projects cloned use cmake to create your build of `b2d-samples`, the rest should be handled automatically.

Resources
---------

Some applications require files that are stored in `resources` directory to work properly. Copying all resources to the build directory should be handled by the build script, but if it fails for whatever reason just copy the content of `resources` to your build directory. If these files are missing the sample should report an error.

All files within resources should be freely redistributable:

  - resources/texture.jpeg (Public Domain) - downloaded from [freestocktextures.com](https://freestocktextures.com), if you need a texture it's a nice place to find one!
  - resources/NotoSans-Regular.ttf (OFL Version 1.1) - downloaded from [Google WebFonts Repo](https://github.com/google/fonts/).
  - src/b2d-sample-tiger.h - Exctacted data from tiger.svg, taken from [amanithvg](http://www.amanithvg.com/). The purpose of this sample is to compare the performance of rendering tiger from such data and compare that to amanithvg tiger demo. Since `b2d-sample-tiger.h` was taken from a project that is not open source it's not recommended to use this file for anything else than this test. In the future this file should be replaced with something that has clear license.

License
-------

All code samples (.cpp files) can be distributed under either Public Domain (UNLICENSE) or ZLIB license. Data files such as `b2d-sample-tiger.h` should not be used outside of this repository.

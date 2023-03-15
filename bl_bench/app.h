// Blend2D - 2D Vector Graphics Powered by a JIT Compiler
//
//  * Official Blend2D Home Page: https://blend2d.com
//  * Official Github Repository: https://github.com/blend2d/blend2d
//
// Copyright (c) 2017-2020 The Blend2D Authors
//
// This software is provided 'as-is', without any express or implied
// warranty. In no event will the authors be held liable for any damages
// arising from the use of this software.
//
// Permission is granted to anyone to use this software for any purpose,
// including commercial applications, and to alter it and redistribute it
// freely, subject to the following restrictions:
//
// 1. The origin of this software must not be misrepresented; you must not
//    claim that you wrote the original software. If you use this software
//    in a product, an acknowledgment in the product documentation would be
//    appreciated but is not required.
// 2. Altered source versions must be plainly marked as such, and must not be
//    misrepresented as being the original software.
// 3. This notice may not be removed or altered from any source distribution.

#ifndef BLBENCH_APP_H
#define BLBENCH_APP_H

#include <blend2d.h>
#include "./module.h"

namespace blbench {

// ============================================================================
// [bench::BenchApp]
// ============================================================================

struct BenchApp {
  // Command line
  int _argc;
  char** _argv;

  // Configuration.
  bool _isolated;
  bool _deepBench;
  bool _saveImages;
  uint32_t _compOp;
  uint32_t _repeat;
  uint32_t _quantity;

  // Assets.
  BLImage _sprites[4];

  // --------------------------------------------------------------------------
  // [Construction / Destruction]
  // --------------------------------------------------------------------------

  BenchApp(int argc, char** argv);
  ~BenchApp();

  // --------------------------------------------------------------------------
  // [Args]
  // --------------------------------------------------------------------------

  bool hasArg(const char* key) const;
  const char* valueOf(const char* key) const;
  int intValueOf(const char* key, int defaultValue) const;

  // --------------------------------------------------------------------------
  // [Init / Info]
  // --------------------------------------------------------------------------

  bool init();
  void info();

  bool readImage(BLImage&, const char* name, const void* data, size_t size) noexcept;

  // --------------------------------------------------------------------------
  // [Helpers]
  // --------------------------------------------------------------------------

  bool isStyleEnabled(uint32_t style);

  // --------------------------------------------------------------------------
  // [Run]
  // --------------------------------------------------------------------------

  int run();
  int runModule(BenchModule& mod, BenchParams& params);
};

} // {blbench}

#endif // BLBENCH_APP_H

// This file is part of Blend2D project <https://blend2d.com>
//
// See LICENSE.md for license and copyright information
// SPDX-License-Identifier: Zlib

#ifndef BLBENCH_APP_H
#define BLBENCH_APP_H

#include <blend2d.h>
#include "./module.h"

namespace blbench {

struct BenchApp {
  // Command line
  int _argc {};
  char** _argv {};

  // Configuration.
  bool _isolated {};
  bool _deepBench {};
  bool _saveImages {};
  uint32_t _compOp {};
  uint32_t _repeat {};
  uint32_t _quantity {};
  const char* _compOpString {};

  // Assets.
  BLImage _sprites[4];

  BenchApp(int argc, char** argv);
  ~BenchApp();

  bool hasArg(const char* key) const;
  const char* valueOf(const char* key) const;
  int intValueOf(const char* key, int defaultValue) const;

  bool init();
  void info();

  bool readImage(BLImage&, const char* name, const void* data, size_t size) noexcept;

  bool isStyleEnabled(uint32_t style);

  int run();
  int runModule(BenchModule& mod, BenchParams& params);
};

} // {blbench}

#endif // BLBENCH_APP_H

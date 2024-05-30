// This file is part of Blend2D project <https://blend2d.com>
//
// See LICENSE.md for license and copyright information
// SPDX-License-Identifier: Zlib

#ifndef BLBENCH_APP_H
#define BLBENCH_APP_H

#include <blend2d.h>
#include "module.h"
#include "jsonbuilder.h"

#include "array"
#include "unordered_map"

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

  bool _disableAgg {};
  bool _disableBlend2D {};
  bool _disableCairo {};
  bool _disableQt {};
  bool _disableSkia {};

  // Assets.
  using SpriteData = std::array<BLImage, 4>;

  SpriteData _spriteData;
  mutable std::unordered_map<uint32_t, SpriteData> _scaledSprites;

  BenchApp(int argc, char** argv);
  ~BenchApp();

  bool hasArg(const char* key) const;
  const char* valueOf(const char* key) const;
  int intValueOf(const char* key, int defaultValue) const;

  bool init();
  void info();

  bool readImage(BLImage&, const char* name, const void* data, size_t size) noexcept;

  BLImage getScaledSprite(uint32_t id, uint32_t size) const;

  bool isStyleEnabled(uint32_t style);

  void serializeSystemInfo(JSONBuilder& json) const;
  void serializeParams(JSONBuilder& json, const BenchParams& params) const;
  void serializeOptions(JSONBuilder& json, const BenchParams& params) const;

  int run();
  int runModule(BenchModule& mod, BenchParams& params, JSONBuilder& json);
  uint64_t runSingleTest(BenchModule& mod, BenchParams& params);
};

} // {blbench}

#endif // BLBENCH_APP_H

// This file is part of Blend2D project <https://blend2d.com>
//
// See LICENSE.md for license and copyright information
// SPDX-License-Identifier: Zlib

#ifndef BLBENCH_APP_H
#define BLBENCH_APP_H

#include "backend.h"
#include "cmdline.h"
#include "jsonbuilder.h"

#include <blend2d.h>

#include <array>
#include <unordered_map>

namespace blbench {

struct BenchApp {
  CmdLine _cmdLine;

  // Configuration.
  uint32_t _width = 512;
  uint32_t _height = 600;
  uint32_t _compOp = 0xFFFFFFFF;
  uint32_t _sizeCount = kBenchShapeSizeCount;
  uint32_t _quantity = 0;
  uint32_t _repeat = 1;
  uint32_t _backends = 0xFFFFFFFF;

  bool _saveImages = false;
  bool _saveOverview = false;

  bool _isolated = false;
  bool _deepBench = false;

  // Assets.
  using SpriteData = std::array<BLImage, 4>;

  SpriteData _spriteData;
  mutable std::unordered_map<uint32_t, SpriteData> _scaledSprites;

  BenchApp(int argc, char** argv);
  ~BenchApp();

  void printAppInfo() const;
  void printOptions() const;
  void printBackends() const;

  bool parseCommandLine();
  bool init();
  void info();

  bool readImage(BLImage&, const char* name, const void* data, size_t size) noexcept;

  BLImage getScaledSprite(uint32_t id, uint32_t size) const;

  bool isBackendEnabled(BackendKind backendKind) const;
  bool isStyleEnabled(StyleKind style) const;

  void serializeSystemInfo(JSONBuilder& json) const;
  void serializeParams(JSONBuilder& json, const BenchParams& params) const;
  void serializeOptions(JSONBuilder& json, const BenchParams& params) const;

  int run();
  int runBackendTests(Backend& backend, BenchParams& params, JSONBuilder& json);
  uint64_t runSingleTest(Backend& backend, BenchParams& params);
};

} // {blbench}

#endif // BLBENCH_APP_H

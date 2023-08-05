// This file is part of Blend2D project <https://blend2d.com>
//
// See LICENSE.md for license and copyright information
// SPDX-License-Identifier: Zlib

#ifndef BLBENCH_MODULE_H
#define BLBENCH_MODULE_H

#include <blend2d.h>
#include "./module.h"

namespace blbench {

struct BenchApp;

// bench - Constants
// =================

enum BenchMisc {
  kBenchNumSprites = 4
};

enum BenchId : uint32_t {
  kBenchIdFillAlignedRect,
  kBenchIdFillSmoothRect,
  kBenchIdFillRotatedRect,
  kBenchIdFillSmoothRound,
  kBenchIdFillRotatedRound,
  kBenchIdFillTriangle,
  kBenchIdFillPolygon10NZ,
  kBenchIdFillPolygon10EO,
  kBenchIdFillPolygon20NZ,
  kBenchIdFillPolygon20EO,
  kBenchIdFillPolygon40NZ,
  kBenchIdFillPolygon40EO,
  kBenchIdFillShapeWorld,

  kBenchIdStrokeAlignedRect,
  kBenchIdStrokeSmoothRect,
  kBenchIdStrokeRotatedRect,
  kBenchIdStrokeSmoothRound,
  kBenchIdStrokeRotatedRound,
  kBenchIdStrokeTriangle,
  kBenchIdStrokePolygon10,
  kBenchIdStrokePolygon20,
  kBenchIdStrokePolygon40,
  kBenchIdStrokeShapeWorld,

  kBenchIdCount
};

enum BenchStyle : uint32_t {
  kBenchStyleSolid,
  kBenchStyleLinearPad,
  kBenchStyleLinearRepeat,
  kBenchStyleLinearReflect,
  kBenchStyleRadialPad,
  kBenchStyleRadialRepeat,
  kBenchStyleRadialReflect,
  kBenchStyleConic,
  kBenchStylePatternNN,
  kBenchStylePatternBI,

  kBenchStyleCount
};

// bench::BenchParams
// ==================

struct BenchParams {
  uint32_t screenW;
  uint32_t screenH;

  BLFormat format;
  uint32_t quantity;

  uint32_t benchId;
  BLCompOp compOp;
  uint32_t style;
  uint32_t shapeSize;

  double strokeWidth;
};

// bench::BenchRandom
// ==================

struct BenchRandom {
  inline BenchRandom(uint64_t seed)
    : _prng(seed),
      _initial(seed) {}

  BLRandom _prng;
  BLRandom _initial;

  inline void rewind() { _prng = _initial; }

  inline int nextInt() {
    return int(_prng.nextUInt32() & 0x7FFFFFFFu);
  }

  inline int nextInt(int a, int b) {
    return int(nextDouble(double(a), double(b)));
  }

  inline double nextDouble() {
    return _prng.nextDouble();
  }

  inline double nextDouble(double a, double b) {
    return a + _prng.nextDouble() * (b - a);
  }

  inline BLPoint nextPoint(const BLSizeI& bounds) {
    double x = nextDouble(0.0, double(bounds.w));
    double y = nextDouble(0.0, double(bounds.h));
    return BLPoint(x, y);
  }

  inline BLPointI nextIntPoint(const BLSizeI& bounds) {
    int x = nextInt(0, bounds.w);
    int y = nextInt(0, bounds.h);
    return BLPointI(x, y);
  }

  inline void nextRectT(BLRect& out, const BLSize& bounds, double w, double h) {
    double x = nextDouble(0.0, bounds.w - w);
    double y = nextDouble(0.0, bounds.h - h);
    out.reset(x, y, w, h);
  }

  inline void nextRectT(BLRectI& out, const BLSizeI& bounds, int w, int h) {
    int x = nextInt(0, bounds.w - w);
    int y = nextInt(0, bounds.h - h);
    out.reset(x, y, w, h);
  }

  inline BLRect nextRect(const BLSize& bounds, double w, double h) {
    double x = nextDouble(0.0, bounds.w - w);
    double y = nextDouble(0.0, bounds.h - h);
    return BLRect(x, y, w, h);
  }

  inline BLRectI nextRectI(const BLSizeI& bounds, int w, int h) {
    int x = nextInt(0, bounds.w - w);
    int y = nextInt(0, bounds.h - h);
    return BLRectI(x, y, w, h);
  }

  inline BLRgba32 nextRgb32() {
    return BLRgba32(_prng.nextUInt32() | 0xFF000000u);
  }

  inline BLRgba32 nextRgba32() {
    return BLRgba32(_prng.nextUInt32());
  }
};

// bench::BenchModule
// ==================

struct BenchModule {
  //! Module name.
  char _name[64];
  //! Current parameters.
  BenchParams _params;
  //! Current duration.
  uint64_t _duration;

  //! Random number generator for coordinates (points or rectangles).
  BenchRandom _rndCoord;
  //! Random number generator for colors.
  BenchRandom _rndColor;
  //! Random number generator for extras (radius).
  BenchRandom _rndExtra;
  //! Random number generator for sprites.
  uint32_t _rndSpriteId;

  //! Blend surface (used by all modules).
  BLImage _surface;
  //! Sprites.
  BLImage _sprites[kBenchNumSprites];

  BenchModule();
  virtual ~BenchModule();

  void run(const BenchApp& app, const BenchParams& params);

  inline uint32_t nextSpriteId() {
    uint32_t i = _rndSpriteId;
    if (++_rndSpriteId >= kBenchNumSprites)
      _rndSpriteId = 0;
    return i;
  };

  virtual bool supportsCompOp(BLCompOp compOp) const = 0;
  virtual bool supportsStyle(uint32_t style) const = 0;

  virtual void onBeforeRun() = 0;
  virtual void onAfterRun() = 0;

  virtual void onDoRectAligned(bool stroke) = 0;
  virtual void onDoRectSmooth(bool stroke) = 0;
  virtual void onDoRectRotated(bool stroke) = 0;
  virtual void onDoRoundSmooth(bool stroke) = 0;
  virtual void onDoRoundRotated(bool stroke) = 0;
  virtual void onDoPolygon(uint32_t mode, uint32_t complexity) = 0;
  virtual void onDoShape(bool stroke, const BLPoint* pts, size_t count) = 0;
};

} // {blbench}

#endif // BLBENCH_MODULE_H

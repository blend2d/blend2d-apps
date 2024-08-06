// This file is part of Blend2D project <https://blend2d.com>
//
// See LICENSE.md for license and copyright information
// SPDX-License-Identifier: Zlib

#ifndef BLBENCH_BACKEND_H
#define BLBENCH_BACKEND_H

#include <blend2d.h>

#include "jsonbuilder.h"
#include "backend.h"
#include "shape_data.h"

namespace blbench {

struct BenchApp;

// blbench - Constants
// ===================

enum class BackendKind : uint32_t {
  kBlend2D,
  kAGG,
  kCairo,
  kQt,
  kSkia,
  kJUCE,
  kCoreGraphics,

  kMaxValue = kCoreGraphics
};

enum class TestKind : uint32_t {
  kFillAlignedRect,
  kFillSmoothRect,
  kFillRotatedRect,
  kFillSmoothRound,
  kFillRotatedRound,
  kFillTriangle,
  kFillPolygon10NZ,
  kFillPolygon10EO,
  kFillPolygon20NZ,
  kFillPolygon20EO,
  kFillPolygon40NZ,
  kFillPolygon40EO,
  kFillButterfly,
  kFillFish,
  kFillDragon,
  kFillWorld,

  kStrokeAlignedRect,
  kStrokeSmoothRect,
  kStrokeRotatedRect,
  kStrokeSmoothRound,
  kStrokeRotatedRound,
  kStrokeTriangle,
  kStrokePolygon10,
  kStrokePolygon20,
  kStrokePolygon40,
  kStrokeButterfly,
  kStrokeFish,
  kStrokeDragon,
  kStrokeWorld,

  kMaxValue = kStrokeWorld
};

enum class StyleKind : uint32_t {
  kSolid,
  kLinearPad,
  kLinearRepeat,
  kLinearReflect,
  kRadialPad,
  kRadialRepeat,
  kRadialReflect,
  kConic,
  kPatternNN,
  kPatternBI,

  kMaxValue = kPatternBI
};

enum class RenderOp : uint32_t {
  kFillNonZero,
  kFillEvenOdd,
  kStroke
};

static constexpr uint32_t kBackendKindCount = uint32_t(BackendKind::kMaxValue) + 1;
static constexpr uint32_t kTestKindCount = uint32_t(TestKind::kMaxValue) + 1;
static constexpr uint32_t kStyleKindCount = uint32_t(StyleKind::kMaxValue) + 1;
static constexpr uint32_t kBenchNumSprites = 4;
static constexpr uint32_t kBenchShapeSizeCount = 6;

// blbench::BenchParams
// ====================

struct BenchParams {
  uint32_t screenW;
  uint32_t screenH;

  BLFormat format;
  uint32_t quantity;

  TestKind testKind;
  StyleKind style;
  BLCompOp compOp;
  uint32_t shapeSize;

  double strokeWidth;
};

// blbench::BenchRandom
// ====================

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

  inline BLRgba32 nextRgba32(uint32_t mask) {
    return BLRgba32(_prng.nextUInt32() | mask);
  }
};

// blbench::Backend
// ====================

struct Backend {
  //! Module name.
  char _name[64] {};
  //! Current parameters.
  BenchParams _params {};
  //! Current duration.
  uint64_t _duration {};

  //! Random number generator for coordinates (points or rectangles).
  BenchRandom _rndCoord;
  //! Random number generator for colors.
  BenchRandom _rndColor;
  //! Random number generator for extras (radius).
  BenchRandom _rndExtra;
  //! Random number generator for sprites.
  uint32_t _rndSpriteId {};

  //! Blend surface (used by all modules).
  BLImage _surface;
  //! Sprites.
  BLImage _sprites[kBenchNumSprites];

  Backend();
  virtual ~Backend();

  void run(const BenchApp& app, const BenchParams& params);

  inline const char* name() const { return _name; }

  inline uint32_t nextSpriteId() {
    uint32_t i = _rndSpriteId;
    if (++_rndSpriteId >= kBenchNumSprites)
      _rndSpriteId = 0;
    return i;
  };

  virtual void serializeInfo(JSONBuilder& json) const;

  virtual bool supportsCompOp(BLCompOp compOp) const = 0;
  virtual bool supportsStyle(StyleKind style) const = 0;

  virtual void beforeRun() = 0;
  virtual void flush() = 0;
  virtual void afterRun() = 0;

  virtual void renderRectA(RenderOp op) = 0;
  virtual void renderRectF(RenderOp op) = 0;
  virtual void renderRectRotated(RenderOp op) = 0;
  virtual void renderRoundF(RenderOp op) = 0;
  virtual void renderRoundRotated(RenderOp op) = 0;
  virtual void renderPolygon(RenderOp op, uint32_t complexity) = 0;
  virtual void renderShape(RenderOp op, ShapeData shape) = 0;
};

} // {blbench}

#endif // BLBENCH_BACKEND_H

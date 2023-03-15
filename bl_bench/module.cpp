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

#include "./app.h"
#include "./module.h"
#include "./shapes_data.h"

#include <chrono>

namespace blbench {

// ============================================================================
// [bench::BenchModule - Construction / Destruction]
// ============================================================================

BenchModule::BenchModule()
  : _name(),
    _params(),
    _duration(0),
    _rndCoord(0x19AE0DDAE3FA7391ull),
    _rndColor(0x94BD7A499AD10011ull),
    _rndExtra(0x1ABD9CC9CAF0F123ull),
    _rndSpriteId(0) {}
BenchModule::~BenchModule() {}

// ============================================================================
// [bench::BenchModule - Run]
// ============================================================================

static void BenchModule_onDoShapeHelper(BenchModule* mod, bool stroke, uint32_t shapeId) {
  ShapesData shape;
  getShapesData(shape, shapeId);
  mod->onDoShape(stroke, shape.data, shape.count);
}

void BenchModule::run(const BenchApp& app, const BenchParams& params) {
  _params = params;

  _rndCoord.rewind();
  _rndColor.rewind();
  _rndExtra.rewind();
  _rndSpriteId = 0;

  // Initialize the sprites.
  for (uint32_t i = 0; i < kBenchNumSprites; i++) {
    BLImage::scale(
      _sprites[i],
      app._sprites[i],
      BLSizeI(params.shapeSize, params.shapeSize), BL_IMAGE_SCALE_FILTER_BILINEAR);
  }

  onBeforeRun();
  auto start = std::chrono::high_resolution_clock::now();

  switch (_params.benchId) {
    case kBenchIdFillAlignedRect   : onDoRectAligned(false); break;
    case kBenchIdFillSmoothRect    : onDoRectSmooth(false); break;
    case kBenchIdFillRotatedRect   : onDoRectRotated(false); break;
    case kBenchIdFillSmoothRound   : onDoRoundSmooth(false); break;
    case kBenchIdFillRotatedRound  : onDoRoundRotated(false); break;
    case kBenchIdFillTriangle      : onDoPolygon(1, 3); break;
    case kBenchIdFillPolygon10NZ   : onDoPolygon(0, 10); break;
    case kBenchIdFillPolygon10EO   : onDoPolygon(1, 10); break;
    case kBenchIdFillPolygon20NZ   : onDoPolygon(0, 20); break;
    case kBenchIdFillPolygon20EO   : onDoPolygon(1, 20); break;
    case kBenchIdFillPolygon40NZ   : onDoPolygon(0, 40); break;
    case kBenchIdFillPolygon40EO   : onDoPolygon(1, 40); break;
    case kBenchIdFillShapeWorld    : BenchModule_onDoShapeHelper(this, false, ShapesData::kIdWorld); break;

    case kBenchIdStrokeAlignedRect : onDoRectAligned(true); break;
    case kBenchIdStrokeSmoothRect  : onDoRectSmooth(true); break;
    case kBenchIdStrokeRotatedRect : onDoRectRotated(true); break;
    case kBenchIdStrokeSmoothRound : onDoRoundSmooth(true); break;
    case kBenchIdStrokeRotatedRound: onDoRoundRotated(true); break;
    case kBenchIdStrokeTriangle    : onDoPolygon(2, 3); break;
    case kBenchIdStrokePolygon10   : onDoPolygon(2, 10); break;
    case kBenchIdStrokePolygon20   : onDoPolygon(2, 20); break;
    case kBenchIdStrokePolygon40   : onDoPolygon(2, 40); break;
    case kBenchIdStrokeShapeWorld  : BenchModule_onDoShapeHelper(this, true, ShapesData::kIdWorld); break;
  }

  onAfterRun();

  auto end = std::chrono::high_resolution_clock::now();
  std::chrono::duration<double> elapsed = end - start;
  _duration = uint64_t(elapsed.count() * 1000000);
}

} // {blbench}

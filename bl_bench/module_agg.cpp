// This file is part of Blend2D project <https://blend2d.com>
//
// See LICENSE.md for license and copyright information
// SPDX-License-Identifier: Zlib

#ifdef BLEND2D_APPS_ENABLE_AGG

#include "./app.h"
#include "./module_agg.h"

#include <algorithm>
#include "agg2d/agg2d.h"

namespace blbench {

static inline uint32_t toAgg2DBlendMode(BLCompOp compOp) {
  switch (compOp) {
    case BL_COMP_OP_CLEAR      : return uint32_t(Agg2D::BlendClear); break;
    case BL_COMP_OP_SRC_COPY   : return uint32_t(Agg2D::BlendSrc); break;
    case BL_COMP_OP_DST_COPY   : return uint32_t(Agg2D::BlendDst); break;
    case BL_COMP_OP_SRC_OVER   : return uint32_t(Agg2D::BlendSrcOver); break;
    case BL_COMP_OP_DST_OVER   : return uint32_t(Agg2D::BlendDstOver); break;
    case BL_COMP_OP_SRC_IN     : return uint32_t(Agg2D::BlendSrcIn); break;
    case BL_COMP_OP_DST_IN     : return uint32_t(Agg2D::BlendDstIn); break;
    case BL_COMP_OP_SRC_OUT    : return uint32_t(Agg2D::BlendSrcOut); break;
    case BL_COMP_OP_DST_OUT    : return uint32_t(Agg2D::BlendDstOut); break;
    case BL_COMP_OP_SRC_ATOP   : return uint32_t(Agg2D::BlendSrcAtop); break;
    case BL_COMP_OP_DST_ATOP   : return uint32_t(Agg2D::BlendDstAtop); break;
    case BL_COMP_OP_XOR        : return uint32_t(Agg2D::BlendXor); break;
    case BL_COMP_OP_PLUS       : return uint32_t(Agg2D::BlendAdd); break;
    case BL_COMP_OP_MULTIPLY   : return uint32_t(Agg2D::BlendMultiply); break;
    case BL_COMP_OP_SCREEN     : return uint32_t(Agg2D::BlendScreen); break;
    case BL_COMP_OP_OVERLAY    : return uint32_t(Agg2D::BlendOverlay); break;
    case BL_COMP_OP_DARKEN     : return uint32_t(Agg2D::BlendDarken); break;
    case BL_COMP_OP_LIGHTEN    : return uint32_t(Agg2D::BlendLighten); break;
    case BL_COMP_OP_COLOR_DODGE: return uint32_t(Agg2D::BlendColorDodge); break;
    case BL_COMP_OP_COLOR_BURN : return uint32_t(Agg2D::BlendColorBurn); break;
    case BL_COMP_OP_HARD_LIGHT : return uint32_t(Agg2D::BlendHardLight); break;
    case BL_COMP_OP_SOFT_LIGHT : return uint32_t(Agg2D::BlendSoftLight); break;
    case BL_COMP_OP_DIFFERENCE : return uint32_t(Agg2D::BlendDifference); break;
    case BL_COMP_OP_EXCLUSION  : return uint32_t(Agg2D::BlendExclusion); break;

    default:
      return 0xFFFFFFFFu;
  }
}

static inline Agg2D::Color toAgg2DColor(BLRgba32 rgba32) {
  return Agg2D::Color(rgba32.r(), rgba32.g(), rgba32.b(), rgba32.a());
}

struct AggModule : public BenchModule {
  Agg2D _ctx;

  AggModule();
  virtual ~AggModule();

  void renderScanlines(const BLRect& rect, uint32_t style);
  void fillRectAA(int x, int y, int w, int h, uint32_t style);

  template<typename T>
  void rasterizePath(T& path, bool stroke);

  virtual bool supportsCompOp(BLCompOp compOp) const;
  virtual bool supportsStyle(uint32_t style) const;

  virtual void onBeforeRun();
  virtual void onAfterRun();

  void prepareFillStrokeOption(bool stroke);

  template<typename RectT>
  void setupStyle(bool stroke, const RectT& rect);

  virtual void onDoRectAligned(bool stroke);
  virtual void onDoRectSmooth(bool stroke);
  virtual void onDoRectRotated(bool stroke);
  virtual void onDoRoundSmooth(bool stroke);
  virtual void onDoRoundRotated(bool stroke);
  virtual void onDoPolygon(uint32_t mode, uint32_t complexity);
  virtual void onDoShape(bool stroke, const BLPoint* pts, size_t count);
};

AggModule::AggModule() {
  strcpy(_name, "AGG");
}
AggModule::~AggModule() {}

bool AggModule::supportsCompOp(BLCompOp compOp) const {
  return toAgg2DBlendMode(compOp) != 0xFFFFFFFFu;
}

bool AggModule::supportsStyle(uint32_t style) const {
  return style == kBenchStyleSolid         ||
         style == kBenchStyleLinearPad     ||
         style == kBenchStyleLinearRepeat  ||
         style == kBenchStyleLinearReflect ||
         style == kBenchStyleRadialPad     ||
         style == kBenchStyleRadialRepeat  ||
         style == kBenchStyleRadialReflect ;
}

void AggModule::onBeforeRun() {
  int w = int(_params.screenW);
  int h = int(_params.screenH);

  BLImageData surfaceData;
  _surface.create(w, h, BL_FORMAT_PRGB32);
  _surface.makeMutable(&surfaceData);

  _ctx.attach(
    static_cast<unsigned char*>(surfaceData.pixelData),
    unsigned(surfaceData.size.w),
    unsigned(surfaceData.size.h),
    int(surfaceData.stride));

  _ctx.fillEvenOdd(false);
  _ctx.noLine();
  _ctx.blendMode(Agg2D::BlendSrc);
  _ctx.clearAll(Agg2D::Color(0, 0, 0, 0));
  _ctx.blendMode(Agg2D::BlendMode(toAgg2DBlendMode(_params.compOp)));
}

void AggModule::onAfterRun() {
  _ctx.attach(nullptr, 0, 0, 0);
}

void AggModule::prepareFillStrokeOption(bool stroke) {
  if (stroke)
    _ctx.noFill();
  else
    _ctx.noLine();
}

template<typename RectT>
void AggModule::setupStyle(bool stroke, const RectT& rect) {
  switch (_params.style) {
    case kBenchStyleSolid: {
      BLRgba32 color = _rndColor.nextRgba32();
      if (!stroke)
        _ctx.fillColor(toAgg2DColor(color));
      else
        _ctx.lineColor(toAgg2DColor(color));
      break;
    }

    case kBenchStyleLinearPad:
    case kBenchStyleLinearRepeat:
    case kBenchStyleLinearReflect: {
      double x1 = rect.x + rect.w * 0.2;
      double y1 = rect.y + rect.h * 0.2;
      double x2 = rect.x + rect.w * 0.8;
      double y2 = rect.y + rect.h * 0.8;

      BLRgba32 c1 = _rndColor.nextRgba32();
      BLRgba32 c2 = _rndColor.nextRgba32();
      BLRgba32 c3 = _rndColor.nextRgba32();

      if (!stroke)
        _ctx.fillLinearGradient(x1, y1, x2, y2, toAgg2DColor(c1), toAgg2DColor(c2), toAgg2DColor(c3));
      else
        _ctx.lineLinearGradient(x1, y1, x2, y2, toAgg2DColor(c1), toAgg2DColor(c2), toAgg2DColor(c3));
      break;
    }

    case kBenchStyleRadialPad:
    case kBenchStyleRadialRepeat:
    case kBenchStyleRadialReflect: {
      double cx = rect.x + rect.w / 2.0;
      double cy = rect.y + rect.h / 2.0;
      double cr = (rect.w + rect.h) / 4.0;

      BLRgba32 c1 = _rndColor.nextRgba32();
      BLRgba32 c2 = _rndColor.nextRgba32();
      BLRgba32 c3 = _rndColor.nextRgba32();

      if (!stroke)
        _ctx.fillRadialGradient(cx, cy, cr, toAgg2DColor(c1), toAgg2DColor(c2), toAgg2DColor(c3));
      else
        _ctx.lineRadialGradient(cx, cy, cr, toAgg2DColor(c1), toAgg2DColor(c2), toAgg2DColor(c3));
      break;
    }
  }
}

void AggModule::onDoRectAligned(bool stroke) {
  BLSizeI bounds(_params.screenW, _params.screenH);
  uint32_t style = _params.style;
  int wh = _params.shapeSize;

  prepareFillStrokeOption(stroke);

  if (_params.style == kBenchStyleSolid && !stroke) {
    for (uint32_t i = 0, quantity = _params.quantity; i < quantity; i++) {
      BLRectI rect = _rndCoord.nextRectI(bounds, wh, wh);
      _ctx.fillRectangleI(rect.x, rect.y, rect.x + rect.w, rect.y + rect.h, toAgg2DColor(_rndColor.nextRgba32()));
    }
  }
  else {
    for (uint32_t i = 0, quantity = _params.quantity; i < quantity; i++) {
      BLRectI rect = _rndCoord.nextRectI(bounds, wh, wh);
      setupStyle(stroke, rect);
      _ctx.rectangle(rect.x, rect.y, rect.x + rect.w, rect.y + rect.h);
    }
  }
}

void AggModule::onDoRectSmooth(bool stroke) {
  BLSize bounds(_params.screenW, _params.screenH);
  uint32_t style = _params.style;
  double wh = _params.shapeSize;

  prepareFillStrokeOption(stroke);

  for (uint32_t i = 0, quantity = _params.quantity; i < quantity; i++) {
    BLRect rect = _rndCoord.nextRect(bounds, wh, wh);
    setupStyle(stroke, rect);
    _ctx.rectangle(rect.x, rect.y, rect.x + rect.w, rect.y + rect.h);
  }
}

void AggModule::onDoRectRotated(bool stroke) {
  BLSize bounds(_params.screenW, _params.screenH);
  uint32_t style = _params.style;

  double cx = double(_params.screenW) * 0.5;
  double cy = double(_params.screenH) * 0.5;
  double wh = _params.shapeSize;
  double angle = 0.0;

  prepareFillStrokeOption(stroke);

  for (uint32_t i = 0, quantity = _params.quantity; i < quantity; i++, angle += 0.01) {
    BLRect rect(_rndCoord.nextRect(bounds, wh, wh));

    agg::trans_affine affine;
    affine.translate(-cx, -cy);
    affine.rotate(angle);
    affine.translate(cx, cy);

    _ctx.affine(affine);
    setupStyle(stroke, rect);
    _ctx.rectangle(rect.x, rect.y, rect.x + rect.w, rect.y + rect.h);
    _ctx.resetTransformations();
  }
}

void AggModule::onDoRoundSmooth(bool stroke) {
  BLSize bounds(_params.screenW, _params.screenH);
  uint32_t style = _params.style;
  double wh = _params.shapeSize;

  prepareFillStrokeOption(stroke);

  for (uint32_t i = 0, quantity = _params.quantity; i < quantity; i++) {
    BLRect rect = _rndCoord.nextRect(bounds, wh, wh);
    double radius = _rndExtra.nextDouble(4.0, 40.0);

    setupStyle(stroke, rect);
    _ctx.roundedRect(rect.x, rect.y, rect.x + rect.w, rect.y + rect.h, radius);
  }
}

void AggModule::onDoRoundRotated(bool stroke) {
  BLSize bounds(_params.screenW, _params.screenH);
  uint32_t style = _params.style;

  double cx = double(_params.screenW) * 0.5;
  double cy = double(_params.screenH) * 0.5;
  double wh = _params.shapeSize;
  double angle = 0.0;

  prepareFillStrokeOption(stroke);

  for (uint32_t i = 0, quantity = _params.quantity; i < quantity; i++, angle += 0.01) {
    BLRect rect(_rndCoord.nextRect(bounds, wh, wh));
    double radius = _rndExtra.nextDouble(4.0, 40.0);

    agg::trans_affine affine;
    affine.translate(-cx, -cy);
    affine.rotate(angle);
    affine.translate(cx, cy);
    _ctx.affine(affine);

    setupStyle(stroke, rect);
    _ctx.roundedRect(rect.x, rect.y, rect.x + rect.w, rect.y + rect.h, radius);
    _ctx.resetTransformations();
  }
}

void AggModule::onDoPolygon(uint32_t mode, uint32_t complexity) {
  BLSizeI bounds(_params.screenW - _params.shapeSize,
                 _params.screenH - _params.shapeSize);

  uint32_t style = _params.style;
  bool stroke = (mode == 2);
  double wh = _params.shapeSize;

  prepareFillStrokeOption(stroke);
  _ctx.fillEvenOdd(mode == 1);

  Agg2D::DrawPathFlag drawPathFlag = stroke ? Agg2D::StrokeOnly : Agg2D::FillOnly;

  for (uint32_t i = 0, quantity = _params.quantity; i < quantity; i++) {
    BLPoint base(_rndCoord.nextPoint(bounds));

    double x = _rndCoord.nextDouble(base.x, base.x + wh);
    double y = _rndCoord.nextDouble(base.y, base.y + wh);

    _ctx.resetPath();
    _ctx.moveTo(x, y);

    for (uint32_t p = 1; p < complexity; p++) {
      x = _rndCoord.nextDouble(base.x, base.x + wh);
      y = _rndCoord.nextDouble(base.y, base.y + wh);
      _ctx.lineTo(x, y);
    }

    setupStyle(stroke, BLRect(base.x, base.y, wh, wh));
    _ctx.drawPath(drawPathFlag);
  }
}

void AggModule::onDoShape(bool stroke, const BLPoint* pts, size_t count) {
  BLSizeI bounds(_params.screenW - _params.shapeSize,
                 _params.screenH - _params.shapeSize);

  uint32_t style = _params.style;
  double wh = double(_params.shapeSize);

  prepareFillStrokeOption(stroke);
  _ctx.fillEvenOdd(false);

  Agg2D::DrawPathFlag drawPathFlag = stroke ? Agg2D::StrokeOnly : Agg2D::FillOnly;

  for (uint32_t i = 0, quantity = _params.quantity; i < quantity; i++) {
    BLPoint base(_rndCoord.nextPoint(bounds));
    bool start = true;

    _ctx.resetPath();

    for (size_t i = 0; i < count; i++) {
      double x = pts[i].x;
      double y = pts[i].y;

      if (x == -1.0 && y == -1.0) {
        start = true;
        continue;
      }

      if (start) {
        _ctx.moveTo(base.x + x * wh, base.y + y * wh);
        start = false;
      }
      else {
        _ctx.lineTo(base.x + x * wh, base.y + y * wh);
      }
    }

    setupStyle(stroke, BLRect(base.x, base.y, wh, wh));
    _ctx.drawPath(drawPathFlag);
  }
}

BenchModule* createAggModule() {
  return new AggModule();
}

} // {blbench}

#endif // BLEND2D_APPS_ENABLE_AGG

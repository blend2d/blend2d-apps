// This file is part of Blend2D project <https://blend2d.com>
//
// See LICENSE.md for license and copyright information
// SPDX-License-Identifier: Zlib

#ifdef BLEND2D_APPS_ENABLE_AGG

#include "app.h"
#include "backend_agg.h"

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

struct AggModule : public Backend {
  Agg2D _ctx;

  AggModule();
  ~AggModule() override;

  void renderScanlines(const BLRect& rect, StyleKind style);
  void fillRectAA(int x, int y, int w, int h, StyleKind style);

  bool supportsCompOp(BLCompOp compOp) const override;
  bool supportsStyle(StyleKind style) const override;

  void beforeRun() override;
  void flush() override;
  void afterRun() override;

  void prepareFillStrokeOption(RenderOp op);

  template<typename RectT>
  void setupStyle(RenderOp op, const RectT& rect);

  void renderRectA(RenderOp op) override;
  void renderRectF(RenderOp op) override;
  void renderRectRotated(RenderOp op) override;
  void renderRoundF(RenderOp op) override;
  void renderRoundRotated(RenderOp op) override;
  void renderPolygon(RenderOp op, uint32_t complexity) override;
  void renderShape(RenderOp op, ShapeData shape) override;
};

AggModule::AggModule() {
  strcpy(_name, "AGG");
}
AggModule::~AggModule() {}

bool AggModule::supportsCompOp(BLCompOp compOp) const {
  return toAgg2DBlendMode(compOp) != 0xFFFFFFFFu;
}

bool AggModule::supportsStyle(StyleKind style) const {
  return style == StyleKind::kSolid         ||
         style == StyleKind::kLinearPad     ||
         style == StyleKind::kLinearRepeat  ||
         style == StyleKind::kLinearReflect ||
         style == StyleKind::kRadialPad     ||
         style == StyleKind::kRadialRepeat  ||
         style == StyleKind::kRadialReflect ;
}

void AggModule::beforeRun() {
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

void AggModule::flush() {
  // Nothing...
}

void AggModule::afterRun() {
  _ctx.attach(nullptr, 0, 0, 0);
}

void AggModule::prepareFillStrokeOption(RenderOp op) {
  if (op == RenderOp::kStroke)
    _ctx.noFill();
  else
    _ctx.noLine();
}

template<typename RectT>
void AggModule::setupStyle(RenderOp op, const RectT& rect) {
  switch (_params.style) {
    case StyleKind::kSolid: {
      BLRgba32 color = _rndColor.nextRgba32();
      if (op == RenderOp::kStroke)
        _ctx.lineColor(toAgg2DColor(color));
      else
        _ctx.fillColor(toAgg2DColor(color));
      break;
    }

    case StyleKind::kLinearPad:
    case StyleKind::kLinearRepeat:
    case StyleKind::kLinearReflect: {
      double x1 = rect.x + rect.w * 0.2;
      double y1 = rect.y + rect.h * 0.2;
      double x2 = rect.x + rect.w * 0.8;
      double y2 = rect.y + rect.h * 0.8;

      BLRgba32 c1 = _rndColor.nextRgba32();
      BLRgba32 c2 = _rndColor.nextRgba32();
      BLRgba32 c3 = _rndColor.nextRgba32();

      if (op == RenderOp::kStroke)
        _ctx.lineLinearGradient(x1, y1, x2, y2, toAgg2DColor(c1), toAgg2DColor(c2), toAgg2DColor(c3));
      else
        _ctx.fillLinearGradient(x1, y1, x2, y2, toAgg2DColor(c1), toAgg2DColor(c2), toAgg2DColor(c3));
      break;
    }

    case StyleKind::kRadialPad:
    case StyleKind::kRadialRepeat:
    case StyleKind::kRadialReflect: {
      double cx = rect.x + rect.w / 2.0;
      double cy = rect.y + rect.h / 2.0;
      double cr = (rect.w + rect.h) / 4.0;

      BLRgba32 c1 = _rndColor.nextRgba32();
      BLRgba32 c2 = _rndColor.nextRgba32();
      BLRgba32 c3 = _rndColor.nextRgba32();

      if (op == RenderOp::kStroke)
        _ctx.lineRadialGradient(cx, cy, cr, toAgg2DColor(c1), toAgg2DColor(c2), toAgg2DColor(c3));
      else
        _ctx.fillRadialGradient(cx, cy, cr, toAgg2DColor(c1), toAgg2DColor(c2), toAgg2DColor(c3));
      break;
    }

    default:
      break;
  }
}

void AggModule::renderRectA(RenderOp op) {
  BLSizeI bounds(_params.screenW, _params.screenH);
  StyleKind style = _params.style;
  int wh = _params.shapeSize;

  prepareFillStrokeOption(op);

  if (_params.style == StyleKind::kSolid && op != RenderOp::kStroke) {
    for (uint32_t i = 0, quantity = _params.quantity; i < quantity; i++) {
      BLRectI rect = _rndCoord.nextRectI(bounds, wh, wh);
      _ctx.fillRectangleI(rect.x, rect.y, rect.x + rect.w, rect.y + rect.h, toAgg2DColor(_rndColor.nextRgba32()));
    }
  }
  else {
    for (uint32_t i = 0, quantity = _params.quantity; i < quantity; i++) {
      BLRectI rect = _rndCoord.nextRectI(bounds, wh, wh);
      setupStyle(op, rect);
      _ctx.rectangle(rect.x, rect.y, rect.x + rect.w, rect.y + rect.h);
    }
  }
}

void AggModule::renderRectF(RenderOp op) {
  BLSize bounds(_params.screenW, _params.screenH);
  StyleKind style = _params.style;
  double wh = _params.shapeSize;

  prepareFillStrokeOption(op);

  for (uint32_t i = 0, quantity = _params.quantity; i < quantity; i++) {
    BLRect rect = _rndCoord.nextRect(bounds, wh, wh);
    setupStyle(op, rect);
    _ctx.rectangle(rect.x, rect.y, rect.x + rect.w, rect.y + rect.h);
  }
}

void AggModule::renderRectRotated(RenderOp op) {
  BLSize bounds(_params.screenW, _params.screenH);
  StyleKind style = _params.style;

  double cx = double(_params.screenW) * 0.5;
  double cy = double(_params.screenH) * 0.5;
  double wh = _params.shapeSize;
  double angle = 0.0;

  prepareFillStrokeOption(op);

  for (uint32_t i = 0, quantity = _params.quantity; i < quantity; i++, angle += 0.01) {
    BLRect rect(_rndCoord.nextRect(bounds, wh, wh));

    agg::trans_affine affine;
    affine.translate(-cx, -cy);
    affine.rotate(angle);
    affine.translate(cx, cy);

    _ctx.affine(affine);
    setupStyle(op, rect);
    _ctx.rectangle(rect.x, rect.y, rect.x + rect.w, rect.y + rect.h);
    _ctx.resetTransformations();
  }
}

void AggModule::renderRoundF(RenderOp op) {
  BLSize bounds(_params.screenW, _params.screenH);
  StyleKind style = _params.style;
  double wh = _params.shapeSize;

  prepareFillStrokeOption(op);

  for (uint32_t i = 0, quantity = _params.quantity; i < quantity; i++) {
    BLRect rect = _rndCoord.nextRect(bounds, wh, wh);
    double radius = _rndExtra.nextDouble(4.0, 40.0);

    setupStyle(op, rect);
    _ctx.roundedRect(rect.x, rect.y, rect.x + rect.w, rect.y + rect.h, radius);
  }
}

void AggModule::renderRoundRotated(RenderOp op) {
  BLSize bounds(_params.screenW, _params.screenH);
  StyleKind style = _params.style;

  double cx = double(_params.screenW) * 0.5;
  double cy = double(_params.screenH) * 0.5;
  double wh = _params.shapeSize;
  double angle = 0.0;

  prepareFillStrokeOption(op);

  for (uint32_t i = 0, quantity = _params.quantity; i < quantity; i++, angle += 0.01) {
    BLRect rect(_rndCoord.nextRect(bounds, wh, wh));
    double radius = _rndExtra.nextDouble(4.0, 40.0);

    agg::trans_affine affine;
    affine.translate(-cx, -cy);
    affine.rotate(angle);
    affine.translate(cx, cy);
    _ctx.affine(affine);

    setupStyle(op, rect);
    _ctx.roundedRect(rect.x, rect.y, rect.x + rect.w, rect.y + rect.h, radius);
    _ctx.resetTransformations();
  }
}

void AggModule::renderPolygon(RenderOp op, uint32_t complexity) {
  BLSizeI bounds(_params.screenW - _params.shapeSize,
                 _params.screenH - _params.shapeSize);

  StyleKind style = _params.style;
  double wh = _params.shapeSize;

  prepareFillStrokeOption(op);
  _ctx.fillEvenOdd(op == RenderOp::kFillEvenOdd);

  Agg2D::DrawPathFlag drawPathFlag = op == RenderOp::kStroke ? Agg2D::StrokeOnly : Agg2D::FillOnly;

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

    setupStyle(op, BLRect(base.x, base.y, wh, wh));
    _ctx.drawPath(drawPathFlag);
  }
}

void AggModule::renderShape(RenderOp op, ShapeData shape) {
  BLSizeI bounds(_params.screenW - _params.shapeSize,
                 _params.screenH - _params.shapeSize);

  StyleKind style = _params.style;
  double wh = double(_params.shapeSize);

  prepareFillStrokeOption(op);
  _ctx.fillEvenOdd(op == RenderOp::kFillEvenOdd);

  Agg2D::DrawPathFlag drawPathFlag = op == RenderOp::kStroke ? Agg2D::StrokeOnly : Agg2D::FillOnly;

  for (uint32_t i = 0, quantity = _params.quantity; i < quantity; i++) {
    BLPoint base(_rndCoord.nextPoint(bounds));
    ShapeIterator it(shape);

    _ctx.resetPath();
    while (it.hasCommand()) {
      if (it.isMoveTo()) {
        _ctx.moveTo(base.x + it.x(0) * wh, base.y + it.y(0) * wh);
      }
      else if (it.isLineTo()) {
        _ctx.lineTo(base.x + it.x(0) * wh, base.y + it.y(0) * wh);
      }
      else if (it.isQuadTo()) {
        _ctx.quadricCurveTo(
          base.x + it.x(0) * wh, base.y + it.y(0) * wh,
          base.x + it.x(1) * wh, base.y + it.y(1) * wh
        );
      }
      else if (it.isCubicTo()) {
        _ctx.cubicCurveTo(
          base.x + it.x(0) * wh, base.y + it.y(0) * wh,
          base.x + it.x(1) * wh, base.y + it.y(1) * wh,
          base.x + it.x(2) * wh, base.y + it.y(2) * wh
        );
      }
      else {
        _ctx.closePolygon();
      }
      it.next();
    }

    setupStyle(op, BLRect(base.x, base.y, wh, wh));
    _ctx.drawPath(drawPathFlag);
  }
}

Backend* createAggBackend() {
  return new AggModule();
}

} // {blbench}

#endif // BLEND2D_APPS_ENABLE_AGG

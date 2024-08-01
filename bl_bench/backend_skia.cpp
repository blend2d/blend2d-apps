// This file is part of Blend2D project <https://blend2d.com>
//
// See LICENSE.md for license and copyright information
// SPDX-License-Identifier: Zlib

#ifdef BLEND2D_APPS_ENABLE_SKIA

#include "app.h"
#include "backend_skia.h"

#include <skia/core/SkBitmap.h>
#include <skia/core/SkCanvas.h>
#include <skia/core/SkColor.h>
#include <skia/core/SkImageInfo.h>
#include <skia/core/SkPaint.h>
#include <skia/core/SkPath.h>
#include <skia/core/SkTypes.h>
#include <skia/effects/SkGradientShader.h>

namespace blbench {

static inline SkIRect toSkIRect(const BLRectI& rect) {
  return SkIRect::MakeXYWH(rect.x, rect.y, rect.w, rect.h);
}

static inline SkRect toSkRect(const BLRect& rect) {
  return SkRect::MakeXYWH(SkScalar(rect.x), SkScalar(rect.y), SkScalar(rect.w), SkScalar(rect.h));
}

static uint32_t toSkBlendMode(BLCompOp compOp) {
  switch (compOp) {
    case BL_COMP_OP_SRC_OVER   : return uint32_t(SkBlendMode::kSrcOver);
    case BL_COMP_OP_SRC_COPY   : return uint32_t(SkBlendMode::kSrc);
    case BL_COMP_OP_SRC_IN     : return uint32_t(SkBlendMode::kSrcIn);
    case BL_COMP_OP_SRC_OUT    : return uint32_t(SkBlendMode::kSrcOut);
    case BL_COMP_OP_SRC_ATOP   : return uint32_t(SkBlendMode::kSrcATop);
    case BL_COMP_OP_DST_OVER   : return uint32_t(SkBlendMode::kDstOver);
    case BL_COMP_OP_DST_COPY   : return uint32_t(SkBlendMode::kDst);
    case BL_COMP_OP_DST_IN     : return uint32_t(SkBlendMode::kDstIn);
    case BL_COMP_OP_DST_OUT    : return uint32_t(SkBlendMode::kDstOut);
    case BL_COMP_OP_DST_ATOP   : return uint32_t(SkBlendMode::kDstATop);
    case BL_COMP_OP_XOR        : return uint32_t(SkBlendMode::kXor);
    case BL_COMP_OP_CLEAR      : return uint32_t(SkBlendMode::kClear);
    case BL_COMP_OP_PLUS       : return uint32_t(SkBlendMode::kPlus);
    case BL_COMP_OP_MODULATE   : return uint32_t(SkBlendMode::kModulate);
    case BL_COMP_OP_MULTIPLY   : return uint32_t(SkBlendMode::kMultiply);
    case BL_COMP_OP_SCREEN     : return uint32_t(SkBlendMode::kScreen);
    case BL_COMP_OP_OVERLAY    : return uint32_t(SkBlendMode::kOverlay);
    case BL_COMP_OP_DARKEN     : return uint32_t(SkBlendMode::kDarken);
    case BL_COMP_OP_LIGHTEN    : return uint32_t(SkBlendMode::kLighten);
    case BL_COMP_OP_COLOR_DODGE: return uint32_t(SkBlendMode::kColorDodge);
    case BL_COMP_OP_COLOR_BURN : return uint32_t(SkBlendMode::kColorBurn);
    case BL_COMP_OP_HARD_LIGHT : return uint32_t(SkBlendMode::kHardLight);
    case BL_COMP_OP_SOFT_LIGHT : return uint32_t(SkBlendMode::kSoftLight);
    case BL_COMP_OP_DIFFERENCE : return uint32_t(SkBlendMode::kDifference);
    case BL_COMP_OP_EXCLUSION  : return uint32_t(SkBlendMode::kExclusion);

    default: return 0xFFFFFFFFu;
  }
}

struct SkiaModule final : public Backend {
  SkCanvas* _skCanvas {};
  SkBitmap _skSurface;
  SkBitmap _skSprites[4];

  SkBlendMode _blendMode {};
  SkTileMode _gradientTileMode {};

  SkiaModule();
  ~SkiaModule() override;

  template<typename RectT>
  sk_sp<SkShader> createShader(StyleKind style, const RectT& rect);

  bool supportsCompOp(BLCompOp compOp) const override;
  bool supportsStyle(StyleKind style) const override;

  void beforeRun() override;
  void flush() override;
  void afterRun() override;

  void renderRectA(RenderOp op) override;
  void renderRectF(RenderOp op) override;
  void renderRectRotated(RenderOp op) override;
  void renderRoundF(RenderOp op) override;
  void renderRoundRotated(RenderOp op) override;
  void renderPolygon(RenderOp op, uint32_t complexity) override;
  void renderShape(RenderOp op, ShapeData shape) override;
};

SkiaModule::SkiaModule() {
  strcpy(_name, "Skia");
}
SkiaModule::~SkiaModule() {}

template<typename RectT>
sk_sp<SkShader> SkiaModule::createShader(StyleKind style, const RectT& rect) {
  static const SkScalar positions3[3] = {
    SkScalar(0.0),
    SkScalar(0.5),
    SkScalar(1.0)
  };

  static const SkScalar positions4[4] = {
    SkScalar(0.0),
    SkScalar(0.33),
    SkScalar(0.66),
    SkScalar(1.0)
  };

  switch (style) {
    case StyleKind::kLinearPad:
    case StyleKind::kLinearRepeat:
    case StyleKind::kLinearReflect: {
      SkPoint pts[2] = {
        SkScalar(rect.x + rect.w * 0.2),
        SkScalar(rect.y + rect.h * 0.2),
        SkScalar(rect.x + rect.w * 0.8),
        SkScalar(rect.y + rect.h * 0.8)
      };

      SkColor colors[3] = {
        _rndColor.nextRgba32().value,
        _rndColor.nextRgba32().value,
        _rndColor.nextRgba32().value
      };

      return SkGradientShader::MakeLinear(pts, colors, positions3, 3, _gradientTileMode);
    }

    case StyleKind::kRadialPad:
    case StyleKind::kRadialRepeat:
    case StyleKind::kRadialReflect: {
      double cx = rect.x + rect.w / 2.0;
      double cy = rect.y + rect.h / 2.0;
      double cr = (rect.w + rect.h) / 4.0;
      double fx = cx - cr / 2;
      double fy = cy - cr / 2;

      SkColor colors[3] = {
        _rndColor.nextRgba32().value,
        _rndColor.nextRgba32().value,
        _rndColor.nextRgba32().value
      };

      return SkGradientShader::MakeTwoPointConical(
        SkPoint::Make(SkScalar(cx), SkScalar(cy)),
        SkScalar(cr),
        SkPoint::Make(SkScalar(fx), SkScalar(fy)),
        SkScalar(0.0),
        colors, positions3, 3, _gradientTileMode);
    }

    case StyleKind::kConic: {
      double cx = rect.x + rect.w / 2;
      double cy = rect.y + rect.h / 2;
      BLRgba32 c = _rndColor.nextRgba32();

      SkColor colors[4] = {
        c.value,
        _rndColor.nextRgba32().value,
        _rndColor.nextRgba32().value,
        c.value
      };

      return SkGradientShader::MakeSweep(SkScalar(cx), SkScalar(cy), colors, positions4, 4);
    }

    case StyleKind::kPatternNN:
    case StyleKind::kPatternBI: {
      uint32_t spriteId = nextSpriteId();
      SkFilterMode filterMode = style == StyleKind::kPatternNN ? SkFilterMode::kNearest : SkFilterMode::kLinear;
      SkMatrix m = SkMatrix::Translate(SkScalar(rect.x), SkScalar(rect.y));
      return _skSprites[spriteId].makeShader(SkSamplingOptions(filterMode), m);
    }

    default: {
      return sk_sp<SkShader>{};
    }
  }
}

bool SkiaModule::supportsCompOp(BLCompOp compOp) const {
  return toSkBlendMode(compOp) != 0xFFFFFFFFu;
}

bool SkiaModule::supportsStyle(StyleKind style) const {
  return style == StyleKind::kSolid         ||
         style == StyleKind::kLinearPad     ||
         style == StyleKind::kLinearRepeat  ||
         style == StyleKind::kLinearReflect ||
         style == StyleKind::kRadialPad     ||
         style == StyleKind::kRadialRepeat  ||
         style == StyleKind::kRadialReflect ||
         style == StyleKind::kConic         ||
         style == StyleKind::kPatternNN     ||
         style == StyleKind::kPatternBI     ;
}

void SkiaModule::beforeRun() {
  int w = int(_params.screenW);
  int h = int(_params.screenH);
  StyleKind style = _params.style;

  // Initialize the sprites.
  for (uint32_t i = 0; i < kBenchNumSprites; i++) {
    const BLImage& sprite = _sprites[i];

    BLImageData spriteData;
    sprite.getData(&spriteData);

    SkImageInfo spriteInfo = SkImageInfo::Make(spriteData.size.w, spriteData.size.h, kBGRA_8888_SkColorType, kPremul_SkAlphaType);
    _skSprites[i].installPixels(spriteInfo, spriteData.pixelData, size_t(spriteData.stride));
  }

  // Initialize the surface and the context.
  BLImageData surfaceData;
  _surface.create(w, h, _params.format);
  _surface.makeMutable(&surfaceData);

  SkImageInfo surfaceInfo = SkImageInfo::Make(w, h, kBGRA_8888_SkColorType, kPremul_SkAlphaType);
  _skSurface.installPixels(surfaceInfo, surfaceData.pixelData, size_t(surfaceData.stride));
  _skSurface.erase(0x00000000, SkIRect::MakeXYWH(0, 0, w, h));

  _skCanvas = new SkCanvas(_skSurface);

  // Setup globals.
  _blendMode = SkBlendMode(toSkBlendMode(_params.compOp));
  _gradientTileMode = SkTileMode::kClamp;

  switch (style) {
    case StyleKind::kLinearPad    : _gradientTileMode = SkTileMode::kClamp ; break;
    case StyleKind::kLinearRepeat : _gradientTileMode = SkTileMode::kRepeat; break;
    case StyleKind::kLinearReflect: _gradientTileMode = SkTileMode::kMirror; break;
    case StyleKind::kRadialPad    : _gradientTileMode = SkTileMode::kClamp ; break;
    case StyleKind::kRadialRepeat : _gradientTileMode = SkTileMode::kRepeat; break;
    case StyleKind::kRadialReflect: _gradientTileMode = SkTileMode::kMirror; break;

    default:
      break;
  }
}

void SkiaModule::flush() {
  // Nothing...
}

void SkiaModule::afterRun() {
  delete _skCanvas;
  _skCanvas = nullptr;
  _skSurface.reset();

  for (uint32_t i = 0; i < kBenchNumSprites; i++) {
    _skSprites[i].reset();
  }
}

void SkiaModule::renderRectA(RenderOp op) {
  BLSizeI bounds(_params.screenW, _params.screenH);
  StyleKind style = _params.style;
  int wh = _params.shapeSize;

  SkPaint p;
  p.setStyle(op == RenderOp::kStroke ? SkPaint::kStroke_Style : SkPaint::kFill_Style);
  p.setAntiAlias(true);
  p.setBlendMode(_blendMode);
  p.setStrokeWidth(SkScalar(_params.strokeWidth));

  if (style == StyleKind::kSolid) {
    for (uint32_t i = 0, quantity = _params.quantity; i < quantity; i++) {
      BLRectI rect = _rndCoord.nextRectI(bounds, wh, wh);

      p.setColor(_rndColor.nextRgba32().value);
      _skCanvas->drawIRect(toSkIRect(rect), p);
    }
  }
  else {
    for (uint32_t i = 0, quantity = _params.quantity; i < quantity; i++) {
      BLRectI rect = _rndCoord.nextRectI(bounds, wh, wh);

      p.setShader(createShader(style, rect));
      _skCanvas->drawIRect(toSkIRect(rect), p);
    }
  }
}

void SkiaModule::renderRectF(RenderOp op) {
  BLSize bounds(_params.screenW, _params.screenH);
  StyleKind style = _params.style;
  double wh = _params.shapeSize;

  SkPaint p;
  p.setStyle(op == RenderOp::kStroke ? SkPaint::kStroke_Style : SkPaint::kFill_Style);
  p.setAntiAlias(true);
  p.setBlendMode(_blendMode);
  p.setStrokeWidth(SkScalar(_params.strokeWidth));

  if (style == StyleKind::kSolid) {
    for (uint32_t i = 0, quantity = _params.quantity; i < quantity; i++) {
      BLRect rect = _rndCoord.nextRect(bounds, wh, wh);

      p.setColor(_rndColor.nextRgba32().value);
      _skCanvas->drawRect(toSkRect(rect), p);
    }
  }
  else {
    for (uint32_t i = 0, quantity = _params.quantity; i < quantity; i++) {
      BLRect rect = _rndCoord.nextRect(bounds, wh, wh);

      p.setShader(createShader(style, rect));
      _skCanvas->drawRect(toSkRect(rect), p);
    }
  }
}

void SkiaModule::renderRectRotated(RenderOp op) {
  BLSize bounds(_params.screenW, _params.screenH);
  StyleKind style = _params.style;

  double cx = double(_params.screenW) * 0.5;
  double cy = double(_params.screenH) * 0.5;
  double wh = _params.shapeSize;
  double angle = 0.0;

  SkPaint p;
  p.setStyle(op == RenderOp::kStroke ? SkPaint::kStroke_Style : SkPaint::kFill_Style);
  p.setAntiAlias(true);
  p.setBlendMode(_blendMode);
  p.setStrokeWidth(SkScalar(_params.strokeWidth));

  for (uint32_t i = 0, quantity = _params.quantity; i < quantity; i++, angle += 0.01) {
    BLRect rect = _rndCoord.nextRect(bounds, wh, wh);

    _skCanvas->rotate(SkRadiansToDegrees(angle), SkScalar(cx), SkScalar(cy));

    if (style == StyleKind::kSolid)
      p.setColor(_rndColor.nextRgba32().value);
    else
      p.setShader(createShader(style, rect));

    _skCanvas->drawRect(toSkRect(rect), p);
    _skCanvas->resetMatrix();
  }
}

void SkiaModule::renderRoundF(RenderOp op) {
  BLSize bounds(_params.screenW, _params.screenH);
  StyleKind style = _params.style;
  double wh = _params.shapeSize;

  SkPaint p;
  p.setStyle(op == RenderOp::kStroke ? SkPaint::kStroke_Style : SkPaint::kFill_Style);
  p.setAntiAlias(true);
  p.setBlendMode(_blendMode);
  p.setStrokeWidth(SkScalar(_params.strokeWidth));

  for (uint32_t i = 0, quantity = _params.quantity; i < quantity; i++) {
    BLRect rect = _rndCoord.nextRect(bounds, wh, wh);
    double radius = _rndExtra.nextDouble(4.0, 40.0);

    if (style == StyleKind::kSolid)
      p.setColor(_rndColor.nextRgba32().value);
    else
      p.setShader(createShader(style, rect));

    _skCanvas->drawRoundRect(toSkRect(rect), SkScalar(radius), SkScalar(radius), p);
  }
}

void SkiaModule::renderRoundRotated(RenderOp op) {
  BLSize bounds(_params.screenW, _params.screenH);
  StyleKind style = _params.style;

  double cx = double(_params.screenW) * 0.5;
  double cy = double(_params.screenH) * 0.5;
  double wh = _params.shapeSize;
  double angle = 0.0;

  SkPaint p;
  p.setStyle(op == RenderOp::kStroke ? SkPaint::kStroke_Style : SkPaint::kFill_Style);
  p.setAntiAlias(true);
  p.setBlendMode(_blendMode);
  p.setStrokeWidth(SkScalar(_params.strokeWidth));

  for (uint32_t i = 0, quantity = _params.quantity; i < quantity; i++, angle += 0.01) {
    _skCanvas->rotate(SkRadiansToDegrees(angle), SkScalar(cx), SkScalar(cy));

    BLRect rect = _rndCoord.nextRect(bounds, wh, wh);
    double radius = _rndExtra.nextDouble(4.0, 40.0);

    if (style == StyleKind::kSolid)
      p.setColor(_rndColor.nextRgba32().value);
    else
      p.setShader(createShader(style, rect));

    _skCanvas->drawRoundRect(toSkRect(rect), SkScalar(radius), SkScalar(radius), p);
    _skCanvas->resetMatrix();
  }
}

void SkiaModule::renderPolygon(RenderOp op, uint32_t complexity) {
  BLSizeI bounds(_params.screenW - _params.shapeSize,
                 _params.screenH - _params.shapeSize);
  StyleKind style = _params.style;
  double wh = double(_params.shapeSize);

  enum { kPointCapacity = 128 };
  if (complexity > kPointCapacity)
    return;

  SkPoint points[kPointCapacity];

  SkPaint p;
  p.setStyle(op == RenderOp::kStroke ? SkPaint::kStroke_Style : SkPaint::kFill_Style);
  p.setAntiAlias(true);
  p.setBlendMode(_blendMode);
  p.setStrokeWidth(SkScalar(_params.strokeWidth));

  // SKIA cannot draw a polygon without having a path, so we have two cases here.
  if (op != RenderOp::kStroke) {
    SkPathFillType fillType = op == RenderOp::kFillEvenOdd ? SkPathFillType::kEvenOdd : SkPathFillType::kWinding;

    for (uint32_t i = 0, quantity = _params.quantity; i < quantity; i++) {
      BLPoint base(_rndCoord.nextPoint(bounds));

      SkPath path;
      path.setFillType(fillType);

      double x, y;
      x = _rndCoord.nextDouble(base.x, base.x + wh);
      y = _rndCoord.nextDouble(base.y, base.y + wh);
      path.moveTo(SkPoint::Make(SkScalar(x), SkScalar(y)));

      for (uint32_t j = 1; j < complexity; j++) {
        x = _rndCoord.nextDouble(base.x, base.x + wh);
        y = _rndCoord.nextDouble(base.y, base.y + wh);
        path.lineTo(SkPoint::Make(SkScalar(x), SkScalar(y)));
      }

      if (style == StyleKind::kSolid) {
        p.setColor(_rndColor.nextRgba32().value);
      }
      else {
        BLRect rect(base.x, base.y, wh, wh);
        p.setShader(createShader(style, rect));
      }

      _skCanvas->drawPath(path, p);
    }
  }
  else {
    for (uint32_t i = 0, quantity = _params.quantity; i < quantity; i++) {
      BLPoint base(_rndCoord.nextPoint(bounds));

      for (uint32_t j = 0; j < complexity; j++) {
        double x = _rndCoord.nextDouble(base.x, base.x + wh);
        double y = _rndCoord.nextDouble(base.y, base.y + wh);
        points[j].set(SkScalar(x), SkScalar(y));
      }

      if (style == StyleKind::kSolid) {
        p.setColor(_rndColor.nextRgba32().value);
      }
      else {
        BLRect rect(base.x, base.y, wh, wh);
        p.setShader(createShader(style, rect));
      }

      _skCanvas->drawPoints(SkCanvas::kPolygon_PointMode, complexity, points, p);
    }
  }
}

void SkiaModule::renderShape(RenderOp op, ShapeData shape) {
  BLSizeI bounds(_params.screenW - _params.shapeSize,
                 _params.screenH - _params.shapeSize);
  StyleKind style = _params.style;
  double wh = double(_params.shapeSize);

  SkPathFillType fillType = op == RenderOp::kFillEvenOdd ? SkPathFillType::kEvenOdd : SkPathFillType::kWinding;

  SkPath path;
  path.setFillType(fillType);

  ShapeIterator it(shape);
  while (it.hasCommand()) {
    if (it.isMoveTo()) {
      path.moveTo(SkScalar(it.x(0) * wh), SkScalar(it.y(0) * wh));
    }
    else if (it.isLineTo()) {
      path.lineTo(SkScalar(it.x(0) * wh), SkScalar(it.y(0) * wh));
    }
    else if (it.isQuadTo()) {
      path.quadTo(
        SkScalar(it.x(0) * wh), SkScalar(it.y(0) * wh),
        SkScalar(it.x(1) * wh), SkScalar(it.y(1) * wh));
    }
    else if (it.isCubicTo()) {
      path.cubicTo(
        SkScalar(it.x(0) * wh), SkScalar(it.y(0) * wh),
        SkScalar(it.x(1) * wh), SkScalar(it.y(1) * wh),
        SkScalar(it.x(2) * wh), SkScalar(it.y(2) * wh));
    }
    else {
      path.close();
    }
    it.next();
  }

  SkPaint p;
  p.setStyle(op == RenderOp::kStroke ? SkPaint::kStroke_Style : SkPaint::kFill_Style);
  p.setAntiAlias(true);
  p.setBlendMode(_blendMode);
  p.setStrokeWidth(SkScalar(_params.strokeWidth));

  for (uint32_t i = 0, quantity = _params.quantity; i < quantity; i++) {
    BLPoint base(_rndCoord.nextPoint(bounds));

    _skCanvas->translate(SkScalar(base.x), SkScalar(base.y));

    if (style == StyleKind::kSolid) {
      p.setColor(_rndColor.nextRgba32().value);
    }
    else {
      BLRect rect(base.x, base.y, wh, wh);
      p.setShader(createShader(style, rect));
    }

    _skCanvas->drawPath(path, p);
    _skCanvas->resetMatrix();
  }
}

Backend* createSkiaBackend() {
  return new SkiaModule();
}

} // {blbench}

#endif // BLEND2D_APPS_ENABLE_SKIA

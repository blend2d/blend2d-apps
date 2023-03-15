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
#include "./module_blend2d.h"

#include <stdio.h>

namespace blbench {

// ============================================================================
// [bench::Blend2DModule - Construction / Destruction]
// ============================================================================

Blend2DModule::Blend2DModule(uint32_t threadCount, uint32_t cpuFeatures) {
  _threadCount = threadCount;
  _cpuFeatures = cpuFeatures;

  const char* feature = nullptr;

  if (_cpuFeatures & BL_RUNTIME_CPU_FEATURE_X86_SSE2  ) feature = "[SSE2]";
  if (_cpuFeatures & BL_RUNTIME_CPU_FEATURE_X86_SSE3  ) feature = "[SSE3]";
  if (_cpuFeatures & BL_RUNTIME_CPU_FEATURE_X86_SSSE3 ) feature = "[SSSE3]";
  if (_cpuFeatures & BL_RUNTIME_CPU_FEATURE_X86_SSE4_1) feature = "[SSE4.1]";
  if (_cpuFeatures & BL_RUNTIME_CPU_FEATURE_X86_SSE4_2) feature = "[SSE4.2]";
  if (_cpuFeatures & BL_RUNTIME_CPU_FEATURE_X86_AVX   ) feature = "[AVX]";
  if (_cpuFeatures & BL_RUNTIME_CPU_FEATURE_X86_AVX2  ) feature = "[AVX2]";

  if (!_threadCount)
    snprintf(_name, sizeof(_name), "Blend2D ST%s%s", feature ? " " : "", feature ? feature : "");
  else
    snprintf(_name, sizeof(_name), "Blend2D %uT%s%s", _threadCount, feature ? " " : "", feature ? feature : "");
}
Blend2DModule::~Blend2DModule() {}

// ============================================================================
// [bench::Blend2DModule - Helpers]
// ============================================================================

template<typename RectT>
static void BlendUtil_setupGradient(Blend2DModule* self, BLGradient& gradient, uint32_t style, const RectT& rect) {
  switch (style) {
    case kBenchStyleLinearPad:
    case kBenchStyleLinearRepeat:
    case kBenchStyleLinearReflect: {
      BLLinearGradientValues values;
      values.x0 = rect.x + rect.w * 0.2;
      values.y0 = rect.y + rect.h * 0.2;
      values.x1 = rect.x + rect.w * 0.8;
      values.y1 = rect.y + rect.h * 0.8;

      gradient.setValues(values);
      gradient.resetStops();
      gradient.addStop(0.0, self->_rndColor.nextRgba32());
      gradient.addStop(0.5, self->_rndColor.nextRgba32());
      gradient.addStop(1.0, self->_rndColor.nextRgba32());
      break;
    }

    case kBenchStyleRadialPad:
    case kBenchStyleRadialRepeat:
    case kBenchStyleRadialReflect: {
      BLRadialGradientValues values;
      values.x0 = rect.x + (rect.w / 2);
      values.y0 = rect.y + (rect.h / 2);
      values.r0 = (rect.w + rect.h) / 4;
      values.x1 = values.x0 - values.r0 / 2.0;
      values.y1 = values.y0 - values.r0 / 2.0;

      gradient.setValues(values);
      gradient.resetStops();
      gradient.addStop(0.0, self->_rndColor.nextRgba32());
      gradient.addStop(0.5, self->_rndColor.nextRgba32());
      gradient.addStop(1.0, self->_rndColor.nextRgba32());
      break;
    }

    default: {
      BLConicalGradientValues values;
      values.x0 = rect.x + (rect.w / 2);
      values.y0 = rect.y + (rect.h / 2);
      values.angle = 0;

      gradient.setValues(values);
      gradient.resetStops();

      BLRgba32 c(self->_rndColor.nextRgba32());

      gradient.addStop(0.00, c);
      gradient.addStop(0.33, self->_rndColor.nextRgba32());
      gradient.addStop(0.66, self->_rndColor.nextRgba32());
      gradient.addStop(1.00, c);
      break;
    }
  }
}

// ============================================================================
// [bench::Blend2DModule - Interface]
// ============================================================================

bool Blend2DModule::supportsCompOp(uint32_t compOp) const {
  return true;
}

bool Blend2DModule::supportsStyle(uint32_t style) const {
  return style == kBenchStyleSolid         ||
         style == kBenchStyleLinearPad     ||
         style == kBenchStyleLinearRepeat  ||
         style == kBenchStyleLinearReflect ||
         style == kBenchStyleRadialPad     ||
         style == kBenchStyleRadialRepeat  ||
         style == kBenchStyleRadialReflect ||
         style == kBenchStyleConical       ||
         style == kBenchStylePatternNN     ||
         style == kBenchStylePatternBI     ;
}

void Blend2DModule::onBeforeRun() {
  int w = int(_params.screenW);
  int h = int(_params.screenH);
  uint32_t style = _params.style;

  BLContextCreateInfo createInfo {};
  createInfo.threadCount = _threadCount;

  if (_cpuFeatures) {
    createInfo.flags = BL_CONTEXT_CREATE_FLAG_ISOLATED_JIT_RUNTIME |
                       BL_CONTEXT_CREATE_FLAG_OVERRIDE_CPU_FEATURES;
    createInfo.cpuFeatures = _cpuFeatures;
  }

  _surface.create(w, h, _params.format);
  _context.begin(_surface, &createInfo);

  _context.setCompOp(BL_COMP_OP_SRC_COPY);
  _context.setFillStyle(BLRgba32(0x00000000));
  _context.fillAll();

  _context.setCompOp(_params.compOp);
  _context.setStrokeWidth(_params.strokeWidth);

  _context.setPatternQuality(
    _params.style == kBenchStylePatternNN
      ? BL_PATTERN_QUALITY_NEAREST
      : BL_PATTERN_QUALITY_BILINEAR);

  // Setup globals.
  _gradientType = BL_GRADIENT_TYPE_LINEAR;
  _gradientExtend = BL_EXTEND_MODE_PAD;

  switch (style) {
    case kBenchStyleLinearPad      : _gradientType = BL_GRADIENT_TYPE_LINEAR ; _gradientExtend = BL_EXTEND_MODE_PAD    ; break;
    case kBenchStyleLinearRepeat   : _gradientType = BL_GRADIENT_TYPE_LINEAR ; _gradientExtend = BL_EXTEND_MODE_REPEAT ; break;
    case kBenchStyleLinearReflect  : _gradientType = BL_GRADIENT_TYPE_LINEAR ; _gradientExtend = BL_EXTEND_MODE_REFLECT; break;
    case kBenchStyleRadialPad      : _gradientType = BL_GRADIENT_TYPE_RADIAL ; _gradientExtend = BL_EXTEND_MODE_PAD    ; break;
    case kBenchStyleRadialRepeat   : _gradientType = BL_GRADIENT_TYPE_RADIAL ; _gradientExtend = BL_EXTEND_MODE_REPEAT ; break;
    case kBenchStyleRadialReflect  : _gradientType = BL_GRADIENT_TYPE_RADIAL ; _gradientExtend = BL_EXTEND_MODE_REFLECT; break;
    case kBenchStyleConical        : _gradientType = BL_GRADIENT_TYPE_CONICAL; break;
  }

  _context.flush(BL_CONTEXT_FLUSH_SYNC);
}

void Blend2DModule::onAfterRun() {
  _context.end();
}

void Blend2DModule::onDoRectAligned(bool stroke) {
  BLSizeI bounds(_params.screenW, _params.screenH);

  uint32_t style = _params.style;
  BLContextOpType opType = stroke ? BL_CONTEXT_OP_TYPE_STROKE : BL_CONTEXT_OP_TYPE_FILL;

  int wh = _params.shapeSize;

  switch (style) {
    case kBenchStyleSolid: {
      for (uint32_t i = 0, quantity = _params.quantity; i < quantity; i++) {
        BLRectI rect(_rndCoord.nextRectI(bounds, wh, wh));
        BLRgba32 color(_rndColor.nextRgba32());

        _context.setStyle(opType, color);
        if (opType == BL_CONTEXT_OP_TYPE_STROKE)
          _context.strokeRect(BLRect(rect.x + 0.5, rect.y + 0.5, rect.w, rect.h));
        else
          _context.fillRect(rect);
      }
      break;
    }

    case kBenchStyleLinearPad:
    case kBenchStyleLinearRepeat:
    case kBenchStyleLinearReflect:
    case kBenchStyleRadialPad:
    case kBenchStyleRadialRepeat:
    case kBenchStyleRadialReflect:
    case kBenchStyleConical: {
      BLGradient gradient(_gradientType);
      gradient.setExtendMode(_gradientExtend);

      for (uint32_t i = 0, quantity = _params.quantity; i < quantity; i++) {
        BLRectI rect(_rndCoord.nextRectI(bounds, wh, wh));
        BlendUtil_setupGradient<BLRectI>(this, gradient, style, rect);

        _context.save();
        _context.setStyle(opType, gradient);

        if (opType == BL_CONTEXT_OP_TYPE_STROKE)
          _context.strokeRect(BLRect(rect.x + 0.5, rect.y + 0.5, rect.w, rect.h));
        else
          _context.fillRect(rect);

        _context.restore();
      }
      break;
    }

    case kBenchStylePatternNN:
    case kBenchStylePatternBI: {
      BLPattern pattern;

      for (uint32_t i = 0, quantity = _params.quantity; i < quantity; i++) {
        BLRectI rect(_rndCoord.nextRectI(bounds, wh, wh));

        if (opType == BL_CONTEXT_OP_TYPE_STROKE) {
          pattern.create(_sprites[nextSpriteId()]);
          pattern.setMatrix(BLMatrix2D::makeTranslation(rect.x + 0.5, rect.y + 0.5));

          _context.save();
          _context.setStrokeStyle(pattern);
          _context.strokeRect(BLRect(rect.x + 0.5, rect.y + 0.5, rect.w, rect.h));
          _context.restore();
        }
        else {
          _context.blitImage(BLPointI(rect.x, rect.y), _sprites[nextSpriteId()]);
        }
      }
      break;
    }
  }
}

void Blend2DModule::onDoRectSmooth(bool stroke) {
  BLSize bounds(_params.screenW, _params.screenH);

  uint32_t style = _params.style;
  BLContextOpType opType = stroke ? BL_CONTEXT_OP_TYPE_STROKE : BL_CONTEXT_OP_TYPE_FILL;

  double wh = _params.shapeSize;

  switch (style) {
    case kBenchStyleSolid: {
      for (uint32_t i = 0, quantity = _params.quantity; i < quantity; i++) {
        BLRect rect(_rndCoord.nextRect(bounds, wh, wh));
        BLRgba32 color(_rndColor.nextRgba32());

        _context.setStyle(opType, color);
        if (opType == BL_CONTEXT_OP_TYPE_STROKE)
          _context.strokeRect(rect);
        else
          _context.fillRect(rect);
      }
      break;
    }

    case kBenchStyleLinearPad:
    case kBenchStyleLinearRepeat:
    case kBenchStyleLinearReflect:
    case kBenchStyleRadialPad:
    case kBenchStyleRadialRepeat:
    case kBenchStyleRadialReflect:
    case kBenchStyleConical: {
      BLGradient gradient(_gradientType);
      gradient.setExtendMode(_gradientExtend);

      for (uint32_t i = 0, quantity = _params.quantity; i < quantity; i++) {
        BLRect rect(_rndCoord.nextRect(bounds, wh, wh));
        BlendUtil_setupGradient<BLRect>(this, gradient, style, rect);

        _context.save();
        _context.setStyle(opType, gradient);

        if (opType == BL_CONTEXT_OP_TYPE_STROKE)
          _context.strokeRect(rect);
        else
          _context.fillRect(rect);

        _context.restore();
      }
      break;
    }

    case kBenchStylePatternNN:
    case kBenchStylePatternBI: {
      BLPattern pattern;

      for (uint32_t i = 0, quantity = _params.quantity; i < quantity; i++) {
        BLRect rect(_rndCoord.nextRect(bounds, wh, wh));

        if (opType == BL_CONTEXT_OP_TYPE_STROKE) {
          pattern.create(_sprites[nextSpriteId()]);
          pattern.setMatrix(BLMatrix2D::makeTranslation(rect.x + 0.5, rect.y + 0.5));

          _context.save();
          _context.setStrokeStyle(pattern);
          _context.strokeRect(BLRect(rect.x + 0.5, rect.y + 0.5, rect.w, rect.h));
          _context.restore();
        }
        else {
          _context.blitImage(BLPoint(rect.x, rect.y), _sprites[nextSpriteId()]);
        }
      }
      break;
    }
  }
}

void Blend2DModule::onDoRectRotated(bool stroke) {
  BLSize bounds(_params.screenW, _params.screenH);

  uint32_t style = _params.style;
  BLContextOpType opType = stroke ? BL_CONTEXT_OP_TYPE_STROKE : BL_CONTEXT_OP_TYPE_FILL;

  double cx = double(_params.screenW) * 0.5;
  double cy = double(_params.screenH) * 0.5;
  double wh = _params.shapeSize;
  double angle = 0.0;

  switch (style) {
    case kBenchStyleSolid: {
      for (uint32_t i = 0, quantity = _params.quantity; i < quantity; i++, angle += 0.01) {
        BLRect rect(_rndCoord.nextRect(bounds, wh, wh));
        BLRgba32 color(_rndColor.nextRgba32());

        _context.save();
        _context.rotate(angle, BLPoint(cx, cy));
        _context.setStyle(opType, color);

        if (opType == BL_CONTEXT_OP_TYPE_STROKE)
          _context.strokeRect(rect);
        else
          _context.fillRect(rect);

        _context.restore();
      }
      break;
    }

    case kBenchStyleLinearPad:
    case kBenchStyleLinearRepeat:
    case kBenchStyleLinearReflect:
    case kBenchStyleRadialPad:
    case kBenchStyleRadialRepeat:
    case kBenchStyleRadialReflect:
    case kBenchStyleConical: {
      BLGradient gradient(_gradientType);
      gradient.setExtendMode(_gradientExtend);

      for (uint32_t i = 0, quantity = _params.quantity; i < quantity; i++, angle += 0.01) {
        BLRect rect(_rndCoord.nextRect(bounds, wh, wh));
        BlendUtil_setupGradient<BLRect>(this, gradient, style, rect);

        _context.save();
        _context.rotate(angle, BLPoint(cx, cy));
        _context.setStyle(opType, gradient);

        if (opType == BL_CONTEXT_OP_TYPE_STROKE)
          _context.strokeRect(rect);
        else
          _context.fillRect(rect);

        _context.restore();
      }
      break;
    }

    case kBenchStylePatternNN:
    case kBenchStylePatternBI: {
      BLPattern pattern;

      for (uint32_t i = 0, quantity = _params.quantity; i < quantity; i++, angle += 0.01) {
        BLRect rect(_rndCoord.nextRect(bounds, wh, wh));

        _context.save();
        if (opType == BL_CONTEXT_OP_TYPE_STROKE) {
          pattern.create(_sprites[nextSpriteId()]);
          pattern.setMatrix(BLMatrix2D::makeTranslation(rect.x + 0.5, rect.y + 0.5));

          _context.setStrokeStyle(pattern);
          _context.rotate(angle, BLPoint(cx, cy));
          _context.strokeRect(BLRect(rect.x + 0.5, rect.y + 0.5, rect.w, rect.h));
        }
        else {
          _context.rotate(angle, BLPoint(cx, cy));
          _context.blitImage(BLPoint(rect.x, rect.y), _sprites[nextSpriteId()]);
        }
        _context.restore();
      }
      break;
    }
  }
}

void Blend2DModule::onDoRoundSmooth(bool stroke) {
  BLSize bounds(_params.screenW, _params.screenH);

  uint32_t style = _params.style;
  BLContextOpType opType = stroke ? BL_CONTEXT_OP_TYPE_STROKE : BL_CONTEXT_OP_TYPE_FILL;

  double wh = _params.shapeSize;

  switch (style) {
    case kBenchStyleSolid: {
      for (uint32_t i = 0, quantity = _params.quantity; i < quantity; i++) {
        double radius = _rndExtra.nextDouble(4.0, 40.0);

        BLRect rect(_rndCoord.nextRect(bounds, wh, wh));
        BLRoundRect round(rect, radius);
        BLRgba32 color(_rndColor.nextRgba32());

        _context.setStyle(opType, color);

        if (opType == BL_CONTEXT_OP_TYPE_STROKE)
          _context.strokeRoundRect(round);
        else
          _context.fillRoundRect(round);
      }
      break;
    }

    case kBenchStyleLinearPad:
    case kBenchStyleLinearRepeat:
    case kBenchStyleLinearReflect:
    case kBenchStyleRadialPad:
    case kBenchStyleRadialRepeat:
    case kBenchStyleRadialReflect:
    case kBenchStyleConical: {
      BLGradient gradient(_gradientType);
      gradient.setExtendMode(_gradientExtend);

      for (uint32_t i = 0, quantity = _params.quantity; i < quantity; i++) {
        double radius = _rndExtra.nextDouble(4.0, 40.0);

        BLRect rect(_rndCoord.nextRect(bounds, wh, wh));
        BLRoundRect round(rect, radius);

        BlendUtil_setupGradient<BLRect>(this, gradient, style, rect);
        _context.save();
        _context.setStyle(opType, gradient);

        if (opType == BL_CONTEXT_OP_TYPE_STROKE)
          _context.strokeRoundRect(round);
        else
          _context.fillRoundRect(round);

        _context.restore();
      }
      break;
    }

    case kBenchStylePatternNN:
    case kBenchStylePatternBI: {
      BLPattern pattern;

      for (uint32_t i = 0, quantity = _params.quantity; i < quantity; i++) {
        double radius = _rndExtra.nextDouble(4.0, 40.0);

        BLRect rect(_rndCoord.nextRect(bounds, wh, wh));
        BLRoundRect round(rect, radius);

        pattern.create(_sprites[nextSpriteId()]);
        pattern.setMatrix(BLMatrix2D::makeTranslation(rect.x, rect.y));

        _context.save();
        _context.setStyle(opType, pattern);

        if (opType == BL_CONTEXT_OP_TYPE_STROKE)
          _context.strokeRoundRect(round);
        else
          _context.fillRoundRect(round);

        _context.restore();
      }
      break;
    }
  }
}

void Blend2DModule::onDoRoundRotated(bool stroke) {
  BLSize bounds(_params.screenW, _params.screenH);

  uint32_t style = _params.style;
  BLContextOpType opType = stroke ? BL_CONTEXT_OP_TYPE_STROKE : BL_CONTEXT_OP_TYPE_FILL;

  double cx = double(_params.screenW) * 0.5;
  double cy = double(_params.screenH) * 0.5;
  double wh = _params.shapeSize;
  double angle = 0.0;

  switch (style) {
    case kBenchStyleSolid: {
      for (uint32_t i = 0, quantity = _params.quantity; i < quantity; i++, angle += 0.01) {
        double radius = _rndExtra.nextDouble(4.0, 40.0);

        BLRect rect(_rndCoord.nextRect(bounds, wh, wh));
        BLRoundRect round(rect, radius);
        BLRgba32 color(_rndColor.nextRgba32());

        _context.save();
        _context.rotate(angle, BLPoint(cx, cy));
        _context.setStyle(opType, color);

        if (opType == BL_CONTEXT_OP_TYPE_STROKE)
          _context.strokeRoundRect(round);
        else
          _context.fillRoundRect(round);

        _context.restore();
      }
      break;
    }

    case kBenchStyleLinearPad:
    case kBenchStyleLinearRepeat:
    case kBenchStyleLinearReflect:
    case kBenchStyleRadialPad:
    case kBenchStyleRadialRepeat:
    case kBenchStyleRadialReflect:
    case kBenchStyleConical: {
      BLGradient gradient(_gradientType);
      gradient.setExtendMode(_gradientExtend);

      for (uint32_t i = 0, quantity = _params.quantity; i < quantity; i++, angle += 0.01) {
        double radius = _rndExtra.nextDouble(4.0, 40.0);

        BLRect rect(_rndCoord.nextRect(bounds, wh, wh));
        BLRoundRect round(rect, radius);

        BlendUtil_setupGradient<BLRect>(this, gradient, style, rect);

        _context.save();
        _context.rotate(angle, BLPoint(cx, cy));
        _context.setStyle(opType, gradient);

        if (opType == BL_CONTEXT_OP_TYPE_STROKE)
          _context.strokeRoundRect(round);
        else
          _context.fillRoundRect(round);

        _context.restore();
      }
      break;
    }

    case kBenchStylePatternNN:
    case kBenchStylePatternBI: {
      BLPattern pattern;

      for (uint32_t i = 0, quantity = _params.quantity; i < quantity; i++, angle += 0.01) {
        double radius = _rndExtra.nextDouble(4.0, 40.0);

        BLRect rect(_rndCoord.nextRect(bounds, wh, wh));
        BLRoundRect round(rect, radius);

        pattern.create(_sprites[nextSpriteId()]);
        pattern.setMatrix(BLMatrix2D::makeTranslation(rect.x, rect.y));

        _context.save();
        _context.rotate(angle, BLPoint(cx, cy));
        _context.setStyle(opType, pattern);

        if (opType == BL_CONTEXT_OP_TYPE_STROKE)
          _context.strokeRoundRect(round);
        else
          _context.fillRoundRect(round);

        _context.restore();
      }
      break;
    }
  }
}

void Blend2DModule::onDoPolygon(uint32_t mode, uint32_t complexity) {
  BLSizeI bounds(_params.screenW - _params.shapeSize,
                 _params.screenH - _params.shapeSize);
  uint32_t style = _params.style;
  BLContextOpType opType = mode == 2 ? BL_CONTEXT_OP_TYPE_STROKE : BL_CONTEXT_OP_TYPE_FILL;

  enum { kPointCapacity = 128 };
  if (complexity > kPointCapacity)
    return;

  BLPoint points[kPointCapacity];
  BLGradient gradient(_gradientType);
  BLPattern pattern;

  gradient.setExtendMode(_gradientExtend);

  if (mode == 0) _context.setFillRule(BL_FILL_RULE_NON_ZERO);
  if (mode == 1) _context.setFillRule(BL_FILL_RULE_EVEN_ODD);

  double wh = double(_params.shapeSize);

  for (uint32_t i = 0, quantity = _params.quantity; i < quantity; i++) {
    BLPoint base(_rndCoord.nextPoint(bounds));

    for (uint32_t p = 0; p < complexity; p++) {
      double x = _rndCoord.nextDouble(base.x, base.x + wh);
      double y = _rndCoord.nextDouble(base.y, base.y + wh);
      points[p].reset(x, y);
    }

    _context.save();

    switch (style) {
      case kBenchStyleSolid: {
        _context.setStyle(opType, _rndColor.nextRgba32());
        break;
      }

      case kBenchStyleLinearPad:
      case kBenchStyleLinearRepeat:
      case kBenchStyleLinearReflect:
      case kBenchStyleRadialPad:
      case kBenchStyleRadialRepeat:
      case kBenchStyleRadialReflect:
      case kBenchStyleConical: {
        BLRect rect(base.x, base.y, wh, wh);
        BlendUtil_setupGradient<BLRect>(this, gradient, style, rect);
        _context.setStyle(opType, gradient);
        break;
      }

      case kBenchStylePatternNN:
      case kBenchStylePatternBI: {
        pattern.create(_sprites[nextSpriteId()]);
        pattern.setMatrix(BLMatrix2D::makeTranslation(base.x, base.y));
        _context.setStyle(opType, pattern);
        break;
      }
    }

    if (opType == BL_CONTEXT_OP_TYPE_STROKE)
      _context.strokePolygon(points, complexity);
    else
      _context.fillPolygon(points, complexity);

    _context.restore();
  }
}

void Blend2DModule::onDoShape(bool stroke, const BLPoint* pts, size_t count) {
  BLSizeI bounds(_params.screenW - _params.shapeSize,
                 _params.screenH - _params.shapeSize);
  uint32_t style = _params.style;
  BLContextOpType opType = stroke ? BL_CONTEXT_OP_TYPE_STROKE : BL_CONTEXT_OP_TYPE_FILL;

  BLPath path;
  bool start = true;
  double wh = double(_params.shapeSize);

  for (size_t i = 0; i < count; i++) {
    double x = pts[i].x;
    double y = pts[i].y;

    if (x == -1.0 && y == -1.0) {
      start = true;
      continue;
    }

    if (start) {
      path.moveTo(x * wh, y * wh);
      start = false;
    }
    else {
      path.lineTo(x * wh, y * wh);
    }
  }

  BLGradient gradient(_gradientType);
  gradient.setExtendMode(_gradientExtend);

  BLPattern pattern;

  for (uint32_t i = 0, quantity = _params.quantity; i < quantity; i++) {
    BLPoint base(_rndCoord.nextPoint(bounds));

    _context.save();

    switch (style) {
      case kBenchStyleSolid: {
        _context.setStyle(opType, _rndColor.nextRgba32());
        break;
      }

      case kBenchStyleLinearPad:
      case kBenchStyleLinearRepeat:
      case kBenchStyleLinearReflect:
      case kBenchStyleRadialPad:
      case kBenchStyleRadialRepeat:
      case kBenchStyleRadialReflect:
      case kBenchStyleConical: {
        BLRect rect(base.x, base.y, wh, wh);
        BlendUtil_setupGradient<BLRect>(this, gradient, style, rect);
        _context.setStyle(opType, gradient);
        break;
      }

      case kBenchStylePatternNN:
      case kBenchStylePatternBI: {
        pattern.create(_sprites[nextSpriteId()]);
        pattern.setMatrix(BLMatrix2D::makeTranslation(base.x, base.y));
        _context.setStyle(opType, pattern);
        break;
      }
    }

    _context.translate(base);
    if (opType == BL_CONTEXT_OP_TYPE_STROKE)
      _context.strokePath(path);
    else
      _context.fillPath(path);

    _context.restore();
  }
}

} // {blbench}

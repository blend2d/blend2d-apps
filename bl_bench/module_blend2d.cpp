// This file is part of Blend2D project <https://blend2d.com>
//
// See LICENSE.md for license and copyright information
// SPDX-License-Identifier: Zlib

#include "app.h"
#include "module_blend2d.h"

#include <blend2d.h>
#include <stdio.h>

namespace blbench {

class Blend2DModule : public BenchModule {
public:

  BLContext _context;
  uint32_t _threadCount;
  uint32_t _cpuFeatures;

  // Initialized by beforeRun().
  BLGradientType _gradientType;
  BLExtendMode _gradientExtend;

  // Construction & Destruction
  // --------------------------

  explicit Blend2DModule(uint32_t threadCount = 0, uint32_t cpuFeatures = 0);
  ~Blend2DModule() override;

  // Interface
  // ---------

  void serializeInfo(JSONBuilder& json) const override;

  bool supportsCompOp(BLCompOp compOp) const override;
  bool supportsStyle(StyleKind style) const override;

  void beforeRun() override;
  void flush() override;
  void afterRun() override;

  template<typename RectT>
  inline const BLVar& setupStyle(const RectT& rect, StyleKind style, BLGradient& gradient, BLPattern& pattern);

  void renderRectA(RenderOp op) override;
  void renderRectF(RenderOp op) override;
  void renderRectRotated(RenderOp op) override;
  void renderRoundF(RenderOp op) override;
  void renderRoundRotated(RenderOp op) override;
  void renderPolygon(RenderOp op, uint32_t complexity) override;
  void renderShape(RenderOp op, ShapeData shape) override;
};

Blend2DModule::Blend2DModule(uint32_t threadCount, uint32_t cpuFeatures) {
  _threadCount = threadCount;
  _cpuFeatures = cpuFeatures;

  const char* feature = nullptr;

  if (_cpuFeatures == 0xFFFFFFFFu) {
    feature = "[NO JIT]";
  }
  else {
    if (_cpuFeatures & BL_RUNTIME_CPU_FEATURE_X86_SSE2  ) feature = "[SSE2]";
    if (_cpuFeatures & BL_RUNTIME_CPU_FEATURE_X86_SSE3  ) feature = "[SSE3]";
    if (_cpuFeatures & BL_RUNTIME_CPU_FEATURE_X86_SSSE3 ) feature = "[SSSE3]";
    if (_cpuFeatures & BL_RUNTIME_CPU_FEATURE_X86_SSE4_1) feature = "[SSE4.1]";
    if (_cpuFeatures & BL_RUNTIME_CPU_FEATURE_X86_SSE4_2) feature = "[SSE4.2]";
    if (_cpuFeatures & BL_RUNTIME_CPU_FEATURE_X86_AVX   ) feature = "[AVX]";
    if (_cpuFeatures & BL_RUNTIME_CPU_FEATURE_X86_AVX2  ) feature = "[AVX2]";
    if (_cpuFeatures & BL_RUNTIME_CPU_FEATURE_X86_AVX512) feature = "[AVX512]";
  }

  if (!_threadCount)
    snprintf(_name, sizeof(_name), "Blend2D ST%s%s", feature ? " " : "", feature ? feature : "");
  else
    snprintf(_name, sizeof(_name), "Blend2D %uT%s%s", _threadCount, feature ? " " : "", feature ? feature : "");
}

Blend2DModule::~Blend2DModule() {}

void Blend2DModule::serializeInfo(JSONBuilder& json) const {
  BLRuntimeBuildInfo buildInfo;
  BLRuntime::queryBuildInfo(&buildInfo);

  json.beforeRecord()
      .addKey("version")
      .addStringf("%u.%u.%u", buildInfo.majorVersion, buildInfo.minorVersion, buildInfo.patchVersion);
}

template<typename RectT>
inline const BLVar& Blend2DModule::setupStyle(const RectT& rect, StyleKind style, BLGradient& gradient, BLPattern& pattern) {
  if (style <= StyleKind::kConic) {
    BLRgba32 c0(_rndColor.nextRgba32());
    BLRgba32 c1(_rndColor.nextRgba32());
    BLRgba32 c2(_rndColor.nextRgba32());

    switch (style) {
      case StyleKind::kLinearPad:
      case StyleKind::kLinearRepeat:
      case StyleKind::kLinearReflect: {
        BLLinearGradientValues values {};
        values.x0 = rect.x + rect.w * 0.2;
        values.y0 = rect.y + rect.h * 0.2;
        values.x1 = rect.x + rect.w * 0.8;
        values.y1 = rect.y + rect.h * 0.8;

        gradient.setValues(values);
        gradient.resetStops();
        gradient.addStop(0.0, c0);
        gradient.addStop(0.5, c1);
        gradient.addStop(1.0, c2);
        break;
      }

      case StyleKind::kRadialPad:
      case StyleKind::kRadialRepeat:
      case StyleKind::kRadialReflect: {
        BLRadialGradientValues values {};
        values.x0 = rect.x + (rect.w / 2);
        values.y0 = rect.y + (rect.h / 2);
        values.r0 = (rect.w + rect.h) / 4;
        values.x1 = values.x0 - values.r0 / 2.0;
        values.y1 = values.y0 - values.r0 / 2.0;

        gradient.setValues(values);
        gradient.resetStops();
        gradient.addStop(0.0, c0);
        gradient.addStop(0.5, c1);
        gradient.addStop(1.0, c2);
        break;
      }

      default: {
        BLConicGradientValues values {};
        values.x0 = rect.x + (rect.w / 2);
        values.y0 = rect.y + (rect.h / 2);
        values.angle = 0;
        values.repeat = 1;

        gradient.setValues(values);
        gradient.resetStops();

        gradient.addStop(0.00, c0);
        gradient.addStop(0.33, c1);
        gradient.addStop(0.66, c2);
        gradient.addStop(1.00, c0);
        break;
      }
    }
    return reinterpret_cast<const BLVar&>(gradient);
  }
  else {
    pattern.create(_sprites[nextSpriteId()], BL_EXTEND_MODE_REPEAT, BLMatrix2D::makeTranslation(rect.x, rect.y));
    return reinterpret_cast<const BLVar&>(pattern);
  }
}

bool Blend2DModule::supportsCompOp(BLCompOp compOp) const {
  return true;
}

bool Blend2DModule::supportsStyle(StyleKind style) const {
  return true;
}

void Blend2DModule::beforeRun() {
  int w = int(_params.screenW);
  int h = int(_params.screenH);
  StyleKind style = _params.style;

  BLContextCreateInfo createInfo {};
  createInfo.threadCount = _threadCount;

  if (_cpuFeatures == 0xFFFFFFFFu) {
    createInfo.flags = BL_CONTEXT_CREATE_FLAG_DISABLE_JIT;
  }
  else  if (_cpuFeatures) {
    createInfo.flags = BL_CONTEXT_CREATE_FLAG_ISOLATED_JIT_RUNTIME |
                       BL_CONTEXT_CREATE_FLAG_OVERRIDE_CPU_FEATURES;
    createInfo.cpuFeatures = _cpuFeatures;
  }

  _surface.create(w, h, _params.format);
  _context.begin(_surface, &createInfo);

  _context.setCompOp(BL_COMP_OP_SRC_COPY);
  _context.fillAll(BLRgba32(0x00000000));

  _context.setCompOp(_params.compOp);
  _context.setStrokeWidth(_params.strokeWidth);

  _context.setPatternQuality(
    _params.style == StyleKind::kPatternNN
      ? BL_PATTERN_QUALITY_NEAREST
      : BL_PATTERN_QUALITY_BILINEAR);

  // Setup globals.
  _gradientType = BL_GRADIENT_TYPE_LINEAR;
  _gradientExtend = BL_EXTEND_MODE_PAD;

  switch (style) {
    case StyleKind::kLinearPad    : _gradientType = BL_GRADIENT_TYPE_LINEAR; _gradientExtend = BL_EXTEND_MODE_PAD    ; break;
    case StyleKind::kLinearRepeat : _gradientType = BL_GRADIENT_TYPE_LINEAR; _gradientExtend = BL_EXTEND_MODE_REPEAT ; break;
    case StyleKind::kLinearReflect: _gradientType = BL_GRADIENT_TYPE_LINEAR; _gradientExtend = BL_EXTEND_MODE_REFLECT; break;
    case StyleKind::kRadialPad    : _gradientType = BL_GRADIENT_TYPE_RADIAL; _gradientExtend = BL_EXTEND_MODE_PAD    ; break;
    case StyleKind::kRadialRepeat : _gradientType = BL_GRADIENT_TYPE_RADIAL; _gradientExtend = BL_EXTEND_MODE_REPEAT ; break;
    case StyleKind::kRadialReflect: _gradientType = BL_GRADIENT_TYPE_RADIAL; _gradientExtend = BL_EXTEND_MODE_REFLECT; break;
    case StyleKind::kConic        : _gradientType = BL_GRADIENT_TYPE_CONIC; break;

    default:
      break;
  }

  _context.flush(BL_CONTEXT_FLUSH_SYNC);
}

void Blend2DModule::flush() {
  _context.flush(BL_CONTEXT_FLUSH_SYNC);
}

void Blend2DModule::afterRun() {
  _context.end();
}

void Blend2DModule::renderRectA(RenderOp op) {
  BLSizeI bounds(_params.screenW, _params.screenH);
  StyleKind style = _params.style;
  int wh = _params.shapeSize;

  if (style == StyleKind::kSolid) {
    for (uint32_t i = 0, quantity = _params.quantity; i < quantity; i++) {
      BLRectI rect(_rndCoord.nextRectI(bounds, wh, wh));
      BLRgba32 color(_rndColor.nextRgba32());

      if (op == RenderOp::kStroke)
        _context.strokeRect(BLRect(rect.x, rect.y, rect.w, rect.h), color);
      else
        _context.fillRect(rect, color);
    }
  }
  else if ((style == StyleKind::kPatternNN || style == StyleKind::kPatternBI) && op != RenderOp::kStroke) {
    for (uint32_t i = 0, quantity = _params.quantity; i < quantity; i++) {
      BLRectI rect(_rndCoord.nextRectI(bounds, wh, wh));
      _context.blitImage(BLPointI(rect.x, rect.y), _sprites[nextSpriteId()]);
    }
  }
  else {
    BLPattern pattern;
    BLGradient gradient(_gradientType);
    gradient.setExtendMode(_gradientExtend);

    for (uint32_t i = 0, quantity = _params.quantity; i < quantity; i++) {
      BLRectI rect(_rndCoord.nextRectI(bounds, wh, wh));
      const auto& obj = setupStyle(rect, style, gradient, pattern);

      if (op == RenderOp::kStroke)
        _context.strokeRect(BLRect(rect.x, rect.y, rect.w, rect.h), obj);
      else
        _context.fillRect(rect, obj);
    }
  }
}

void Blend2DModule::renderRectF(RenderOp op) {
  BLSize bounds(_params.screenW, _params.screenH);
  StyleKind style = _params.style;
  double wh = _params.shapeSize;

  if (style == StyleKind::kSolid) {
    for (uint32_t i = 0, quantity = _params.quantity; i < quantity; i++) {
      BLRect rect(_rndCoord.nextRect(bounds, wh, wh));
      BLRgba32 color(_rndColor.nextRgba32());

      if (op == RenderOp::kStroke)
        _context.strokeRect(rect, color);
      else
        _context.fillRect(rect, color);
    }
  }
  else if ((style == StyleKind::kPatternNN || style == StyleKind::kPatternBI) && op != RenderOp::kStroke) {
    for (uint32_t i = 0, quantity = _params.quantity; i < quantity; i++) {
      BLRect rect(_rndCoord.nextRect(bounds, wh, wh));
      _context.blitImage(BLPoint(rect.x, rect.y), _sprites[nextSpriteId()]);
    }
  }
  else {
    BLPattern pattern;
    BLGradient gradient(_gradientType);
    gradient.setExtendMode(_gradientExtend);

    for (uint32_t i = 0, quantity = _params.quantity; i < quantity; i++) {
      BLRect rect(_rndCoord.nextRect(bounds, wh, wh));
      const auto& obj = setupStyle(rect, style, gradient, pattern);

      if (op == RenderOp::kStroke)
        _context.strokeRect(rect, obj);
      else
        _context.fillRect(rect, obj);
    }
  }
}

void Blend2DModule::renderRectRotated(RenderOp op) {
  BLSize bounds(_params.screenW, _params.screenH);
  StyleKind style = _params.style;

  double cx = double(_params.screenW) * 0.5;
  double cy = double(_params.screenH) * 0.5;
  double wh = _params.shapeSize;
  double angle = 0.0;

  if (style == StyleKind::kSolid) {
    for (uint32_t i = 0, quantity = _params.quantity; i < quantity; i++, angle += 0.01) {
      BLRect rect(_rndCoord.nextRect(bounds, wh, wh));
      BLRgba32 color(_rndColor.nextRgba32());

      _context.rotate(angle, BLPoint(cx, cy));

      if (op == RenderOp::kStroke)
        _context.strokeRect(rect, color);
      else
        _context.fillRect(rect, color);

      _context.resetTransform();
    }
  }
  else if ((style == StyleKind::kPatternNN || style == StyleKind::kPatternBI) && op != RenderOp::kStroke) {
    for (uint32_t i = 0, quantity = _params.quantity; i < quantity; i++, angle += 0.01) {
      BLRect rect(_rndCoord.nextRect(bounds, wh, wh));

      _context.save();
      _context.rotate(angle, BLPoint(cx, cy));
      _context.blitImage(BLPoint(rect.x, rect.y), _sprites[nextSpriteId()]);
      _context.restore();
    }
  }
  else {
    BLPattern pattern;
    BLGradient gradient(_gradientType);
    gradient.setExtendMode(_gradientExtend);

    for (uint32_t i = 0, quantity = _params.quantity; i < quantity; i++, angle += 0.01) {
      BLRect rect(_rndCoord.nextRect(bounds, wh, wh));
      const auto& obj = setupStyle(rect, style, gradient, pattern);

      _context.save();
      _context.rotate(angle, BLPoint(cx, cy));

      if (op == RenderOp::kStroke)
        _context.strokeRect(rect, obj);
      else
        _context.fillRect(rect, obj);

      _context.restore();
    }
  }
}

void Blend2DModule::renderRoundF(RenderOp op) {
  BLSize bounds(_params.screenW, _params.screenH);
  StyleKind style = _params.style;
  double wh = _params.shapeSize;

  if (style == StyleKind::kSolid) {
    for (uint32_t i = 0, quantity = _params.quantity; i < quantity; i++) {
      double radius = _rndExtra.nextDouble(4.0, 40.0);
      BLRect rect(_rndCoord.nextRect(bounds, wh, wh));
      BLRoundRect round(rect, radius);

      BLRgba32 color(_rndColor.nextRgba32());
      if (op == RenderOp::kStroke)
        _context.strokeRoundRect(round, color);
      else
        _context.fillRoundRect(round, color);
    }
  }
  else {
    BLPattern pattern;
    BLGradient gradient(_gradientType);
    gradient.setExtendMode(_gradientExtend);

    for (uint32_t i = 0, quantity = _params.quantity; i < quantity; i++) {
      double radius = _rndExtra.nextDouble(4.0, 40.0);
      BLRect rect(_rndCoord.nextRect(bounds, wh, wh));
      BLRoundRect round(rect, radius);

      const auto& obj = setupStyle(rect, style, gradient, pattern);
      if (op == RenderOp::kStroke)
        _context.strokeRoundRect(round, obj);
      else
        _context.fillRoundRect(round, obj);
    }
  }
}

void Blend2DModule::renderRoundRotated(RenderOp op) {
  BLSize bounds(_params.screenW, _params.screenH);
  StyleKind style = _params.style;

  double cx = double(_params.screenW) * 0.5;
  double cy = double(_params.screenH) * 0.5;
  double wh = _params.shapeSize;
  double angle = 0.0;

  if (style == StyleKind::kSolid) {
    for (uint32_t i = 0, quantity = _params.quantity; i < quantity; i++, angle += 0.01) {
      double radius = _rndExtra.nextDouble(4.0, 40.0);
      BLRect rect(_rndCoord.nextRect(bounds, wh, wh));
      BLRoundRect round(rect, radius);

      _context.rotate(angle, BLPoint(cx, cy));
      BLRgba32 color(_rndColor.nextRgba32());

      if (op == RenderOp::kStroke)
        _context.strokeRoundRect(round, color);
      else
        _context.fillRoundRect(round, color);

      _context.resetTransform();
    }
  }
  else {
    BLPattern pattern;
    BLGradient gradient(_gradientType);
    gradient.setExtendMode(_gradientExtend);

    for (uint32_t i = 0, quantity = _params.quantity; i < quantity; i++, angle += 0.01) {
      double radius = _rndExtra.nextDouble(4.0, 40.0);
      BLRect rect(_rndCoord.nextRect(bounds, wh, wh));
      BLRoundRect round(rect, radius);

      const auto& obj = setupStyle(rect, style, gradient, pattern);

      _context.save();
      _context.rotate(angle, BLPoint(cx, cy));

      if (op == RenderOp::kStroke)
        _context.strokeRoundRect(round, obj);
      else
        _context.fillRoundRect(round, obj);

      _context.restore();
    }
  }
}

void Blend2DModule::renderPolygon(RenderOp op, uint32_t complexity) {
  static constexpr uint32_t kPointCapacity = 128;

  if (complexity > kPointCapacity)
    return;

  BLSizeI bounds(_params.screenW - _params.shapeSize, _params.screenH - _params.shapeSize);
  StyleKind style = _params.style;
  double wh = double(_params.shapeSize);

  BLPoint points[kPointCapacity];
  BLPattern pattern;
  BLGradient gradient(_gradientType);

  _context.setFillRule(op == RenderOp::kFillEvenOdd ? BL_FILL_RULE_EVEN_ODD : BL_FILL_RULE_NON_ZERO);
  gradient.setExtendMode(_gradientExtend);

  for (uint32_t i = 0, quantity = _params.quantity; i < quantity; i++) {
    BLPoint base(_rndCoord.nextPoint(bounds));

    for (uint32_t p = 0; p < complexity; p++) {
      double x = _rndCoord.nextDouble(base.x, base.x + wh);
      double y = _rndCoord.nextDouble(base.y, base.y + wh);
      points[p].reset(x, y);
    }

    if (style == StyleKind::kSolid) {
      BLRgba32 color = _rndColor.nextRgba32();
      if (op == RenderOp::kStroke)
        _context.strokePolygon(points, complexity, color);
      else
        _context.fillPolygon(points, complexity, color);
    }
    else {
      BLRect rect(base.x, base.y, wh, wh);
      const auto& obj = setupStyle(rect, style, gradient, pattern);

      if (op == RenderOp::kStroke)
        _context.strokePolygon(points, complexity, obj);
      else
        _context.fillPolygon(points, complexity, obj);
    }
  }
}

void Blend2DModule::renderShape(RenderOp op, ShapeData shape) {
  BLSizeI bounds(_params.screenW - _params.shapeSize, _params.screenH - _params.shapeSize);
  StyleKind style = _params.style;
  double wh = double(_params.shapeSize);

  BLPath path;
  ShapeIterator it(shape);

  while (it.hasCommand()) {
    if (it.isMoveTo()) {
      path.moveTo(it.vertex(0));
    }
    else if (it.isLineTo()) {
      path.lineTo(it.vertex(0));
    }
    else if (it.isQuadTo()) {
      path.quadTo(it.vertex(0), it.vertex(1));
    }
    else if (it.isCubicTo()) {
      path.cubicTo(it.vertex(0), it.vertex(1), it.vertex(2));
    }
    else {
      path.close();
    }
    it.next();
  }

  path.transform(BLMatrix2D::makeScaling(wh, wh));

  BLPattern pattern;
  BLGradient gradient(_gradientType);

  _context.setFillRule(op == RenderOp::kFillEvenOdd ? BL_FILL_RULE_EVEN_ODD : BL_FILL_RULE_NON_ZERO);
  gradient.setExtendMode(_gradientExtend);

  for (uint32_t i = 0, quantity = _params.quantity; i < quantity; i++) {
    BLPoint base(_rndCoord.nextPoint(bounds));

    if (style == StyleKind::kSolid) {
      BLRgba32 color = _rndColor.nextRgba32();
      if (op == RenderOp::kStroke)
        _context.strokePath(base, path, color);
      else
        _context.fillPath(base, path, color);
    }
    else {
      BLRect rect(base.x, base.y, wh, wh);
      const auto& obj = setupStyle(rect, style, gradient, pattern);

      if (op == RenderOp::kStroke)
        _context.strokePath(base, path, obj);
      else
        _context.fillPath(base, path, obj);
    }
  }
}

BenchModule* createBlend2DModule(uint32_t threadCount, uint32_t cpuFeatures) {
  return new Blend2DModule(threadCount, cpuFeatures);
}

} // {blbench}

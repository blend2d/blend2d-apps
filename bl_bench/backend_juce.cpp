// This file is part of Blend2D project <https://blend2d.com>
//
// See LICENSE.md for license and copyright information
// SPDX-License-Identifier: Zlib

#ifdef BLEND2D_APPS_ENABLE_JUCE

#include "app.h"
#include "backend_juce.h"

#include <juce_graphics/juce_graphics.h>

namespace blbench {

static inline juce::Image::PixelFormat toJuceFormat(BLFormat format) noexcept {
  switch (format) {
    case BL_FORMAT_PRGB32:
      return juce::Image::ARGB;

    case BL_FORMAT_XRGB32:
      return juce::Image::RGB;

    case BL_FORMAT_A8:
      return juce::Image::SingleChannel;

    default:
      return juce::Image::UnknownFormat;
  }
}

static inline BLFormat toBlend2DFormat(juce::Image::PixelFormat format) noexcept {
  switch (format) {
    case juce::Image::ARGB:
      return BL_FORMAT_PRGB32;

    case juce::Image::RGB:
      return BL_FORMAT_XRGB32;

    case juce::Image::SingleChannel:
      return BL_FORMAT_A8;

    default:
      return BL_FORMAT_NONE;
  }
}

static void convertBlend2DImageToJuce(juce::Image& dst, BLImage& src, const juce::ImageType& imageType) {
  BLImageData srcData;
  src.getData(&srcData);

  BLFormat format = BLFormat(srcData.format);
  uint32_t w = uint32_t(srcData.size.w);
  uint32_t h = uint32_t(srcData.size.h);
  size_t rowSize = size_t(w) * (blFormatInfo[format].depth / 8);

  if (dst.getWidth() != int(w) && dst.getHeight() != h || dst.getFormat() != toJuceFormat(format)) {
    dst = juce::Image(toJuceFormat(format), int(w), int(h), false, imageType);
  }

  juce::Image::BitmapData dstData(dst, juce::Image::BitmapData::readWrite);

  for (uint32_t y = 0; y < h; y++) {
    memcpy(
      dstData.data + intptr_t(y) * dstData.lineStride,
      static_cast<const uint8_t*>(srcData.pixelData) + intptr_t(y) * srcData.stride,
      rowSize);
  }
}

static void convertJuceImageToBlend2D(BLImage& dst, juce::Image& src) {
  juce::Image::BitmapData srcData(src, juce::Image::BitmapData::readOnly);
  BLFormat format = toBlend2DFormat(srcData.pixelFormat);
  uint32_t w = uint32_t(srcData.width);
  uint32_t h = uint32_t(srcData.height);
  size_t rowSize = size_t(w) * (blFormatInfo[format].depth / 8);

  BLImageData dstData;
  dst.create(int(w), int(h), format);
  dst.makeMutable(&dstData);

  for (uint32_t y = 0; y < h; y++) {
    memcpy(
      static_cast<uint8_t*>(dstData.pixelData) + intptr_t(y) * dstData.stride,
      srcData.data + intptr_t(y) * srcData.lineStride,
      rowSize);
  }
}

static inline juce::Colour toJuceColor(BLRgba32 rgba) noexcept {
  return juce::Colour(
    uint8_t(rgba.r()),
    uint8_t(rgba.g()),
    uint8_t(rgba.b()),
    uint8_t(rgba.a()));
}

struct JuceModule : public Backend {
  juce::SoftwareImageType _juceImageType;
  juce::PathStrokeType _juceStrokeType;
  float _lineThickness {};

  juce::Image _juceSurface;
  juce::Image _juceSprites[kBenchNumSprites];
  juce::Graphics* _juceContext {};

  JuceModule();
  ~JuceModule() override;

  void serializeInfo(JSONBuilder& json) const override;

  bool supportsCompOp(BLCompOp compOp) const override;
  bool supportsStyle(StyleKind style) const override;

  void beforeRun() override;
  void flush() override;
  void afterRun() override;

  template<typename RectT>
  inline void setupStyle(const RectT& rect, StyleKind style);

  void renderRectA(RenderOp op) override;
  void renderRectF(RenderOp op) override;
  void renderRectRotated(RenderOp op) override;
  void renderRoundF(RenderOp op) override;
  void renderRoundRotated(RenderOp op) override;
  void renderPolygon(RenderOp op, uint32_t complexity) override;
  void renderShape(RenderOp op, ShapeData shape) override;
};

JuceModule::JuceModule()
  : _juceStrokeType(1.0f) {
  strcpy(_name, "JUCE");
}

JuceModule::~JuceModule() {}

void JuceModule::serializeInfo(JSONBuilder& json) const {
#if defined(JUCE_VERSION)
  uint32_t majorVersion = (JUCE_VERSION >> 16);
  uint32_t minorVersion = (JUCE_VERSION >> 8) & 0xFF;
  uint32_t patchVersion = (JUCE_VERSION >> 0) & 0xFF;

  json.beforeRecord()
      .addKey("version")
      .addStringf("%u.%u.%u", majorVersion, minorVersion, patchVersion);
#endif // JUCE_VERSION
}

bool JuceModule::supportsCompOp(BLCompOp compOp) const {
  return compOp == BL_COMP_OP_SRC_OVER;
}

bool JuceModule::supportsStyle(StyleKind style) const {
  return style == StyleKind::kSolid         ||
         style == StyleKind::kLinearPad     ||
         style == StyleKind::kLinearRepeat  ||
         style == StyleKind::kLinearReflect ||
         style == StyleKind::kRadialPad     ||
         style == StyleKind::kRadialRepeat  ||
         style == StyleKind::kRadialReflect ||
         style == StyleKind::kPatternNN     ||
         style == StyleKind::kPatternBI     ;
}

void JuceModule::beforeRun() {
  int w = int(_params.screenW);
  int h = int(_params.screenH);
  StyleKind style = _params.style;

  _lineThickness = float(_params.strokeWidth);
  _juceStrokeType.setEndStyle(juce::PathStrokeType::butt);
  _juceStrokeType.setJointStyle(juce::PathStrokeType::mitered);
  _juceStrokeType.setStrokeThickness(_lineThickness);

  for (uint32_t i = 0; i < kBenchNumSprites; i++) {
    convertBlend2DImageToJuce(_juceSprites[i], _sprites[i], _juceImageType);
  }

  // NOTE: There seems to be no way we can use a user provided pixel buffer in JUCE, so let's create two
  // separate buffers so we can copy to our main `_surface` in `afterRun()`. The `afterRun()` function
  // is excluded from rendering time so there is no penalty for JUCE.
  _juceSurface = juce::Image(toJuceFormat(_params.format), w, h, false, _juceImageType);
  _juceSurface.clear(juce::Rectangle<int>(0, 0, w, h));
  _juceContext = new juce::Graphics(_juceSurface);

  if (_params.style == StyleKind::kPatternBI) {
    _juceContext->setImageResamplingQuality(juce::Graphics::mediumResamplingQuality);
  }
  else {
    _juceContext->setImageResamplingQuality(juce::Graphics::lowResamplingQuality);
  }
}

void JuceModule::flush() {
}

void JuceModule::afterRun() {
  if (_juceContext) {
    delete _juceContext;
    _juceContext = nullptr;
  }

  convertJuceImageToBlend2D(_surface, _juceSurface);
}

template<typename RectT>
inline void JuceModule::setupStyle(const RectT& rect, StyleKind style) {
  switch (style) {
    case StyleKind::kLinearPad:
    case StyleKind::kLinearRepeat:
    case StyleKind::kLinearReflect: {
      BLRgba32 c0 = _rndColor.nextRgba32();
      BLRgba32 c1 = _rndColor.nextRgba32();
      BLRgba32 c2 = _rndColor.nextRgba32();

      float x0 = float(rect.x) + rect.w * float(0.2);
      float y0 = float(rect.y) + rect.h * float(0.2);
      float x1 = float(rect.x) + rect.w * float(0.8);
      float y1 = float(rect.y) + rect.h * float(0.8);

      juce::ColourGradient gradient(toJuceColor(c0), juce::Point<float>(x0, y0), toJuceColor(c2), juce::Point<float>(x1, y1), false);
      gradient.addColour(0.5, toJuceColor(c1));
      _juceContext->setGradientFill(gradient);
      break;
    }

    case StyleKind::kRadialPad:
    case StyleKind::kRadialRepeat:
    case StyleKind::kRadialReflect: {
      BLRgba32 c0 = _rndColor.nextRgba32();
      BLRgba32 c1 = _rndColor.nextRgba32();
      BLRgba32 c2 = _rndColor.nextRgba32();

      float cx = float(rect.x + rect.w / 2);
      float cy = float(rect.y + rect.h / 2);
      float cr = float((rect.w + rect.h) / 4);
      float fx = float(cx - cr / 2);
      float fy = float(cy - cr / 2);

      juce::ColourGradient gradient(toJuceColor(c0), juce::Point<float>(cx, cy), toJuceColor(c2), juce::Point<float>(cx - cr, cy - cr), true);
      gradient.addColour(0.5, toJuceColor(c1));
      _juceContext->setGradientFill(gradient);
      break;
    }

    case StyleKind::kPatternNN:
    case StyleKind::kPatternBI: {
      juce::AffineTransform transform = juce::AffineTransform::translation(float(rect.x), float(rect.y));
      _juceContext->setFillType(juce::FillType(_juceSprites[nextSpriteId()], transform));
      break;
    }

    default:
      return;
  }
}

void JuceModule::renderRectA(RenderOp op) {
  BLSizeI bounds(_params.screenW, _params.screenH);
  StyleKind style = _params.style;
  int wh = _params.shapeSize;

  if (style == StyleKind::kSolid) {
    for (uint32_t i = 0, quantity = _params.quantity; i < quantity; i++) {
      BLRectI r = _rndCoord.nextRectI(bounds, wh, wh);
      _juceContext->setColour(toJuceColor(_rndColor.nextRgba32()));

      if (op == RenderOp::kStroke)
        _juceContext->drawRect(juce::Rectangle<int>(r.x, r.y, r.w, r.h), _lineThickness);
      else
        _juceContext->fillRect(juce::Rectangle<int>(r.x, r.y, r.w, r.h));
    }
  }
  else if ((style == StyleKind::kPatternNN || style == StyleKind::kPatternBI) && op != RenderOp::kStroke) {
    for (uint32_t i = 0, quantity = _params.quantity; i < quantity; i++) {
      BLRectI rect(_rndCoord.nextRectI(bounds, wh, wh));
      _juceContext->drawImageAt(_juceSprites[nextSpriteId()], rect.x, rect.y);
    }
  }
  else {
    for (uint32_t i = 0, quantity = _params.quantity; i < quantity; i++) {
      BLRectI r = _rndCoord.nextRectI(bounds, wh, wh);
      setupStyle(r, style);

      if (op == RenderOp::kStroke)
        _juceContext->drawRect(juce::Rectangle<int>(r.x, r.y, r.w, r.h), _lineThickness);
      else
        _juceContext->fillRect(juce::Rectangle<int>(r.x, r.y, r.w, r.h));
    }
  }
}

void JuceModule::renderRectF(RenderOp op) {
  BLSize bounds(_params.screenW, _params.screenH);
  StyleKind style = _params.style;
  double wh = _params.shapeSize;

  if (style == StyleKind::kSolid) {
    for (uint32_t i = 0, quantity = _params.quantity; i < quantity; i++) {
      BLRect r = _rndCoord.nextRect(bounds, wh, wh);
      _juceContext->setColour(toJuceColor(_rndColor.nextRgba32()));

      if (op == RenderOp::kStroke)
        _juceContext->drawRect(juce::Rectangle<float>(float(r.x), float(r.y), float(r.w), float(r.h)), _lineThickness);
      else
        _juceContext->fillRect(juce::Rectangle<float>(float(r.x), float(r.y), float(r.w), float(r.h)));
    }
  }
  else {
    for (uint32_t i = 0, quantity = _params.quantity; i < quantity; i++) {
      BLRect r = _rndCoord.nextRect(bounds, wh, wh);
      setupStyle(r, style);

      if (op == RenderOp::kStroke)
        _juceContext->drawRect(juce::Rectangle<float>(float(r.x), float(r.y), float(r.w), float(r.h)), _lineThickness);
      else
        _juceContext->fillRect(juce::Rectangle<float>(float(r.x), float(r.y), float(r.w), float(r.h)));
    }
  }
}

void JuceModule::renderRectRotated(RenderOp op) {
  BLSize bounds(_params.screenW, _params.screenH);
  StyleKind style = _params.style;

  double cx = double(_params.screenW) * 0.5;
  double cy = double(_params.screenH) * 0.5;
  double wh = _params.shapeSize;
  double angle = 0.0;

  for (uint32_t i = 0, quantity = _params.quantity; i < quantity; i++, angle += 0.01) {
    BLRect r(_rndCoord.nextRect(bounds, wh, wh));
    juce::AffineTransform tr = juce::AffineTransform::rotation(float(angle), float(cx), float(cy));

    _juceContext->saveState();
    _juceContext->addTransform(tr);

    if (style == StyleKind::kSolid) {
      _juceContext->setColour(toJuceColor(_rndColor.nextRgba32()));
    }
    else {
      setupStyle(r, style);
    }

    if (op == RenderOp::kStroke)
      _juceContext->drawRect(juce::Rectangle<float>(float(r.x), float(r.y), float(r.w), float(r.h)), _lineThickness);
    else
      _juceContext->fillRect(juce::Rectangle<float>(float(r.x), float(r.y), float(r.w), float(r.h)));

    _juceContext->restoreState();
  }
}

void JuceModule::renderRoundF(RenderOp op) {
  BLSize bounds(_params.screenW, _params.screenH);
  StyleKind style = _params.style;
  double wh = _params.shapeSize;

  for (uint32_t i = 0, quantity = _params.quantity; i < quantity; i++) {
    BLRect r(_rndCoord.nextRect(bounds, wh, wh));
    float radius = float(_rndExtra.nextDouble(4.0, 40.0));

    if (style == StyleKind::kSolid) {
      _juceContext->setColour(toJuceColor(_rndColor.nextRgba32()));
    }
    else {
      setupStyle(r, style);
    }

    if (op == RenderOp::kStroke)
      _juceContext->drawRoundedRectangle(float(r.x), float(r.y), float(r.w), float(r.h), radius, _lineThickness);
    else
      _juceContext->fillRoundedRectangle(float(r.x), float(r.y), float(r.w), float(r.h), radius);
  }
}

void JuceModule::renderRoundRotated(RenderOp op) {
  BLSize bounds(_params.screenW, _params.screenH);
  StyleKind style = _params.style;

  double cx = double(_params.screenW) * 0.5;
  double cy = double(_params.screenH) * 0.5;
  double wh = _params.shapeSize;
  double angle = 0.0;

  for (uint32_t i = 0, quantity = _params.quantity; i < quantity; i++, angle += 0.01) {
    BLRect r(_rndCoord.nextRect(bounds, wh, wh));
    float radius = float(_rndExtra.nextDouble(4.0, 40.0));
    juce::AffineTransform tr = juce::AffineTransform::rotation(float(angle), float(cx), float(cy));

    _juceContext->saveState();
    _juceContext->addTransform(tr);

    if (style == StyleKind::kSolid) {
      _juceContext->setColour(toJuceColor(_rndColor.nextRgba32()));
    }
    else {
      setupStyle(r, style);
    }

    if (op == RenderOp::kStroke)
      _juceContext->drawRoundedRectangle(float(r.x), float(r.y), float(r.w), float(r.h), radius, _lineThickness);
    else
      _juceContext->fillRoundedRectangle(float(r.x), float(r.y), float(r.w), float(r.h), radius);

    _juceContext->restoreState();
  }
}

void JuceModule::renderPolygon(RenderOp op, uint32_t complexity) {
  BLSizeI bounds(_params.screenW - _params.shapeSize,
                 _params.screenH - _params.shapeSize);
  StyleKind style = _params.style;
  double wh = double(_params.shapeSize);

  juce::Path path;
  path.setUsingNonZeroWinding(op != RenderOp::kFillEvenOdd);

  for (uint32_t i = 0, quantity = _params.quantity; i < quantity; i++) {
    BLPoint base(_rndCoord.nextPoint(bounds));

    double x = _rndCoord.nextDouble(base.x, base.x + wh);
    double y = _rndCoord.nextDouble(base.y, base.y + wh);

    path.clear();
    path.startNewSubPath(float(x), float(y));

    for (uint32_t p = 1; p < complexity; p++) {
      x = _rndCoord.nextDouble(base.x, base.x + wh);
      y = _rndCoord.nextDouble(base.y, base.y + wh);
      path.lineTo(float(x), float(y));
    }

    path.closeSubPath();

    if (style == StyleKind::kSolid) {
      _juceContext->setColour(toJuceColor(_rndColor.nextRgba32()));
    }
    else {
      setupStyle(BLRect(x, y, wh, wh), style);
    }

    if (op == RenderOp::kStroke)
      _juceContext->strokePath(path, _juceStrokeType);
    else
      _juceContext->fillPath(path);
  }
}

void JuceModule::renderShape(RenderOp op, ShapeData shape) {
  BLSizeI bounds(_params.screenW - _params.shapeSize,
                 _params.screenH - _params.shapeSize);
  StyleKind style = _params.style;
  double wh = double(_params.shapeSize);

  juce::Path path;
  path.setUsingNonZeroWinding(op != RenderOp::kFillEvenOdd);

  ShapeIterator it(shape);
  while (it.hasCommand()) {
    if (it.isMoveTo()) {
      path.startNewSubPath(
        float(it.x(0) * wh), float(it.y(0) * wh));
    }
    else if (it.isLineTo()) {
      path.lineTo(
        float(it.x(0) * wh), float(it.y(0) * wh));
    }
    else if (it.isQuadTo()) {
      path.quadraticTo(
        float(it.x(0) * wh), float(it.y(0) * wh),
        float(it.x(1) * wh), float(it.y(1) * wh));
    }
    else if (it.isCubicTo()) {
      path.cubicTo(
        float(it.x(0) * wh), float(it.y(0) * wh),
        float(it.x(1) * wh), float(it.y(1) * wh),
        float(it.x(2) * wh), float(it.y(2) * wh));
    }
    else {
      path.closeSubPath();
    }
    it.next();
  }

  for (uint32_t i = 0, quantity = _params.quantity; i < quantity; i++) {
    BLPoint base(_rndCoord.nextPoint(bounds));
    juce::AffineTransform transform = juce::AffineTransform::translation(float(base.x), float(base.y));

    if (style == StyleKind::kSolid) {
      _juceContext->setColour(toJuceColor(_rndColor.nextRgba32()));
    }
    else {
      setupStyle(BLRect(base.x, base.y, wh, wh), style);
    }

    if (op == RenderOp::kStroke)
      _juceContext->strokePath(path, _juceStrokeType, transform);
    else
      _juceContext->fillPath(path, transform);
  }
}

Backend* createJuceBackend() {
  return new JuceModule();
}

} // {blbench}

#endif // BLEND2D_APPS_ENABLE_QT

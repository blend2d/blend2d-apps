// This file is part of Blend2D project <https://blend2d.com>
//
// See LICENSE.md for license and copyright information
// SPDX-License-Identifier: Zlib

#ifdef BLEND2D_APPS_ENABLE_COREGRAPHICS

#include "app.h"
#include "backend_coregraphics.h"

#include <ApplicationServices/ApplicationServices.h>
#include <utility>

namespace blbench {

static uint32_t toCGBitmapInfo(BLFormat format) noexcept {
  switch (format) {
    case BL_FORMAT_PRGB32: return kCGImageByteOrder32Little | kCGImageAlphaPremultipliedFirst;
    case BL_FORMAT_XRGB32: return kCGImageByteOrder32Little | kCGImageAlphaNoneSkipFirst;

    default: return 0;
  }
}

static CGBlendMode toCGBlendMode(BLCompOp compOp) noexcept {
  switch (compOp) {
    case BL_COMP_OP_SRC_OVER   : return kCGBlendModeNormal;
    case BL_COMP_OP_SRC_COPY   : return kCGBlendModeCopy;
    case BL_COMP_OP_SRC_IN     : return kCGBlendModeSourceIn;
    case BL_COMP_OP_SRC_OUT    : return kCGBlendModeSourceOut;
    case BL_COMP_OP_SRC_ATOP   : return kCGBlendModeSourceAtop;
    case BL_COMP_OP_DST_OVER   : return kCGBlendModeDestinationOver;
    case BL_COMP_OP_DST_IN     : return kCGBlendModeDestinationIn;
    case BL_COMP_OP_DST_OUT    : return kCGBlendModeDestinationOut;
    case BL_COMP_OP_DST_ATOP   : return kCGBlendModeDestinationAtop;
    case BL_COMP_OP_XOR        : return kCGBlendModeXOR;
    case BL_COMP_OP_CLEAR      : return kCGBlendModeClear;
    case BL_COMP_OP_PLUS       : return kCGBlendModePlusLighter;
    case BL_COMP_OP_MULTIPLY   : return kCGBlendModeMultiply;
    case BL_COMP_OP_SCREEN     : return kCGBlendModeScreen;
    case BL_COMP_OP_OVERLAY    : return kCGBlendModeOverlay;
    case BL_COMP_OP_DARKEN     : return kCGBlendModeDarken;
    case BL_COMP_OP_LIGHTEN    : return kCGBlendModeLighten;
    case BL_COMP_OP_COLOR_DODGE: return kCGBlendModeColorDodge;
    case BL_COMP_OP_COLOR_BURN : return kCGBlendModeColorBurn;
    case BL_COMP_OP_HARD_LIGHT : return kCGBlendModeHardLight;
    case BL_COMP_OP_SOFT_LIGHT : return kCGBlendModeSoftLight;
    case BL_COMP_OP_DIFFERENCE : return kCGBlendModeDifference;
    case BL_COMP_OP_EXCLUSION  : return kCGBlendModeExclusion;

    default:
      return kCGBlendModeNormal;
  }
}

template<typename RectT>
static inline CGRect toCGRect(const RectT& rect) noexcept {
  return CGRectMake(CGFloat(rect.x), CGFloat(rect.y), CGFloat(rect.w), CGFloat(rect.h));
}

static inline void toCGColorComponents(CGFloat components[4], BLRgba32 color) noexcept {
  constexpr CGFloat scale = CGFloat(1.0f / 255.0f);
  components[0] = CGFloat(int(color.r())) * scale;
  components[1] = CGFloat(int(color.g())) * scale;
  components[2] = CGFloat(int(color.b())) * scale;
  components[3] = CGFloat(int(color.a())) * scale;
}

static void flipImage(BLImage& img) noexcept {
  BLImageData imgData;
  img.makeMutable(&imgData);

  uint32_t h = uint32_t(imgData.size.h);
  intptr_t stride = imgData.stride;
  uint8_t* pixelData = static_cast<uint8_t*>(imgData.pixelData);

  for (uint32_t y = 0; y < h / 2; y++) {
    uint8_t* a = pixelData + intptr_t(y) * stride;
    uint8_t* b = pixelData + intptr_t(h - y - 1) * stride;

    for (size_t x = 0; x < size_t(stride); x++) {
      std::swap(a[x], b[x]);
    }

  }
}

struct CoreGraphicsModule : public Backend {
  CGImageRef _cgSprites[kBenchNumSprites] {};

  CGColorSpaceRef _cgColorspace {};
  CGContextRef _cgContext {};

  CoreGraphicsModule();
  ~CoreGraphicsModule() override;

  void serializeInfo(JSONBuilder& json) const override;

  bool supportsCompOp(BLCompOp compOp) const override;
  bool supportsStyle(StyleKind style) const override;

  void beforeRun() override;
  void flush() override;
  void afterRun() override;

  CGGradientRef createGradient(StyleKind style) noexcept;

  BL_INLINE void renderSolidPath(RenderOp op) noexcept;

  template<typename RectT>
  BL_INLINE void renderSolidRect(const RectT& rect, RenderOp op) noexcept;

  template<bool kSaveGState, typename RectT>
  BL_INLINE void renderStyledPath(const RectT& rect, StyleKind style, RenderOp op) noexcept;

  template<bool kSaveGState, typename RectT>
  BL_INLINE void renderStyledRect(const RectT& rect, StyleKind style, RenderOp op) noexcept;

  void renderRectA(RenderOp op) override;
  void renderRectF(RenderOp op) override;
  void renderRectRotated(RenderOp op) override;
  void renderRoundF(RenderOp op) override;
  void renderRoundRotated(RenderOp op) override;
  void renderPolygon(RenderOp op, uint32_t complexity) override;
  void renderShape(RenderOp op, ShapeData shape) override;
};

CoreGraphicsModule::CoreGraphicsModule() {
  strcpy(_name, "CoreGraphics");
}

CoreGraphicsModule::~CoreGraphicsModule() {}

void CoreGraphicsModule::serializeInfo(JSONBuilder& json) const {
}

bool CoreGraphicsModule::supportsCompOp(BLCompOp compOp) const {
  return compOp == BL_COMP_OP_SRC_OVER    ||
         compOp == BL_COMP_OP_SRC_COPY    ||
         compOp == BL_COMP_OP_SRC_IN      ||
         compOp == BL_COMP_OP_SRC_OUT     ||
         compOp == BL_COMP_OP_SRC_ATOP    ||
         compOp == BL_COMP_OP_DST_OVER    ||
         compOp == BL_COMP_OP_DST_IN      ||
         compOp == BL_COMP_OP_DST_OUT     ||
         compOp == BL_COMP_OP_DST_ATOP    ||
         compOp == BL_COMP_OP_XOR         ||
         compOp == BL_COMP_OP_CLEAR       ||
         compOp == BL_COMP_OP_PLUS        ||
         compOp == BL_COMP_OP_MULTIPLY    ||
         compOp == BL_COMP_OP_SCREEN      ||
         compOp == BL_COMP_OP_OVERLAY     ||
         compOp == BL_COMP_OP_DARKEN      ||
         compOp == BL_COMP_OP_LIGHTEN     ||
         compOp == BL_COMP_OP_COLOR_DODGE ||
         compOp == BL_COMP_OP_COLOR_BURN  ||
         compOp == BL_COMP_OP_HARD_LIGHT  ||
         compOp == BL_COMP_OP_SOFT_LIGHT  ||
         compOp == BL_COMP_OP_DIFFERENCE  ||
         compOp == BL_COMP_OP_EXCLUSION   ;
}

bool CoreGraphicsModule::supportsStyle(StyleKind style) const {
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

void CoreGraphicsModule::beforeRun() {
  int w = int(_params.screenW);
  int h = int(_params.screenH);
  StyleKind style = _params.style;

  _cgColorspace = CGColorSpaceCreateWithName(kCGColorSpaceGenericRGBLinear);

  // Initialize the sprites.
  for (uint32_t i = 0; i < kBenchNumSprites; i++) {
    BLImage& sprite = _sprites[i];
    flipImage(sprite);

    BLImageData spriteData;
    sprite.getData(&spriteData);

    BLFormat spriteFormat = BLFormat(spriteData.format);

    CGDataProviderRef dp = CGDataProviderCreateWithData(
      nullptr,
      spriteData.pixelData,
      size_t(spriteData.size.h) * size_t(spriteData.stride),
      nullptr);

    if (!dp) {
      printf("Failed to create CGDataProvider\n");
      continue;
    }

    CGImageRef cgSprite = CGImageCreate(
      size_t(spriteData.size.w),      // Width.
      size_t(spriteData.size.h),      // Height.
      8,                              // Bits per component.
      32,                             // Bits per pixel.
      size_t(spriteData.stride),      // Bytes per row.
      _cgColorspace,                  // Colorspace.
      toCGBitmapInfo(spriteFormat),   // Bitmap info.
      dp,                             // Data provider.
      nullptr,                        // Decode.
      style == StyleKind::kPatternBI, // Should interpolate.
      kCGRenderingIntentDefault);     // Color rendering intent.

    if (!cgSprite) {
      printf("Failed to create CGImage\n");
      continue;
    }

    _cgSprites[i] = cgSprite;
    CGDataProviderRelease(dp);
  }

  // Initialize the surface and the context.
  BLImageData surfaceData;
  _surface.create(w, h, _params.format);
  _surface.makeMutable(&surfaceData);

  _cgContext = CGBitmapContextCreate(
    surfaceData.pixelData,
    uint32_t(surfaceData.size.w),
    uint32_t(surfaceData.size.h),
    8,
    size_t(surfaceData.stride),
    _cgColorspace,
    toCGBitmapInfo(BLFormat(surfaceData.format)));

  const CGFloat transparent[4] {};

  // Setup the context.
  CGContextSetBlendMode(_cgContext, kCGBlendModeCopy);
  CGContextSetFillColorSpace(_cgContext, _cgColorspace);
  CGContextSetStrokeColorSpace(_cgContext, _cgColorspace);

  CGContextSetFillColor(_cgContext, transparent);
  CGContextFillRect(_cgContext, CGRectMake(0, 0, CGFloat(surfaceData.size.w), CGFloat(surfaceData.size.h)));

  CGContextSetBlendMode(_cgContext, toCGBlendMode(_params.compOp));
  CGContextSetAllowsAntialiasing(_cgContext, true);

  CGContextSetLineJoin(_cgContext, kCGLineJoinMiter);
  CGContextSetLineWidth(_cgContext, CGFloat(_params.strokeWidth));
}

void CoreGraphicsModule::flush() {
  CGContextSynchronize(_cgContext);
}

void CoreGraphicsModule::afterRun() {
  CGContextRelease(_cgContext);
  CGColorSpaceRelease(_cgColorspace);

  _cgContext = nullptr;
  _cgColorspace = nullptr;

  // Free the sprites.
  for (uint32_t i = 0; i < kBenchNumSprites; i++) {
    if (_cgSprites[i]) {
      CGImageRelease(_cgSprites[i]);
      _cgSprites[i] = nullptr;
    }
  }

  flipImage(_surface);
}

CGGradientRef CoreGraphicsModule::createGradient(StyleKind style) noexcept {
  BLRgba32 c0 = _rndColor.nextRgba32();
  BLRgba32 c1 = _rndColor.nextRgba32();
  BLRgba32 c2 = _rndColor.nextRgba32();

  switch (style) {
    case StyleKind::kLinearPad:
    case StyleKind::kLinearRepeat:
    case StyleKind::kRadialPad:
    case StyleKind::kRadialRepeat: {
      CGFloat components[12];
      toCGColorComponents(components + 0, c0);
      toCGColorComponents(components + 4, c1);
      toCGColorComponents(components + 8, c2);

      CGFloat locations[3] = {
        CGFloat(0.0),
        CGFloat(0.5),
        CGFloat(1.0)
      };

      return CGGradientCreateWithColorComponents(_cgColorspace, components, locations, 3);
    }

    case StyleKind::kLinearReflect:
    case StyleKind::kRadialReflect: {

      CGFloat components[20];
      toCGColorComponents(components +  0, c0);
      toCGColorComponents(components +  4, c1);
      toCGColorComponents(components +  8, c2);
      toCGColorComponents(components + 12, c1);
      toCGColorComponents(components + 16, c0);

      CGFloat locations[5] = {
        CGFloat(0.0),
        CGFloat(0.25),
        CGFloat(0.5),
        CGFloat(0.75),
        CGFloat(1.0)
      };

      return CGGradientCreateWithColorComponents(_cgColorspace, components, locations, 5);
    }

    case StyleKind::kConic: {
      BLRgba32 c0 = _rndColor.nextRgba32();
      BLRgba32 c1 = _rndColor.nextRgba32();
      BLRgba32 c2 = _rndColor.nextRgba32();

      CGFloat components[16];
      toCGColorComponents(components +  0, c0);
      toCGColorComponents(components +  4, c1);
      toCGColorComponents(components +  8, c2);
      toCGColorComponents(components + 12, c0);

      CGFloat locations[4] = {
        CGFloat(0.0),
        CGFloat(0.33),
        CGFloat(0.66),
        CGFloat(1.0)
      };

      return CGGradientCreateWithColorComponents(_cgColorspace, components, locations, 4);
    }

    default:
      return CGGradientRef{};
  }
}

BL_INLINE void CoreGraphicsModule::renderSolidPath(RenderOp op) noexcept {
  CGFloat color[4];
  toCGColorComponents(color, _rndColor.nextRgba32());

  if (op == RenderOp::kStroke) {
    CGContextSetStrokeColor(_cgContext, color);
    CGContextStrokePath(_cgContext);
  }
  else {
    CGContextSetFillColor(_cgContext, color);
    if (op == RenderOp::kFillNonZero)
      CGContextFillPath(_cgContext);
    else
      CGContextEOFillPath(_cgContext);
  }
}

template<typename RectT>
BL_INLINE void CoreGraphicsModule::renderSolidRect(const RectT& rect, RenderOp op) noexcept {
  CGFloat color[4];
  toCGColorComponents(color, _rndColor.nextRgba32());

  if (op == RenderOp::kStroke) {
    CGContextSetStrokeColor(_cgContext, color);
    CGContextStrokeRect(_cgContext, toCGRect(rect));
  }
  else {
    CGContextSetFillColor(_cgContext, color);
    CGContextFillRect(_cgContext, toCGRect(rect));
  }
}

template<bool kSaveGState, typename RectT>
BL_INLINE void CoreGraphicsModule::renderStyledPath(const RectT& rect, StyleKind style, RenderOp op) noexcept {
  if (kSaveGState)
    CGContextSaveGState(_cgContext);

  if (op == RenderOp::kStroke) {
    CGContextReplacePathWithStrokedPath(_cgContext);
  }

  if (op == RenderOp::kFillEvenOdd) {
    CGContextEOClip(_cgContext);
  }
  else {
    CGContextClip(_cgContext);
  }

  switch (style) {
    case StyleKind::kSolid:
      // Not reached (the caller must use renderSolidPath() instead).
      break;

    case StyleKind::kLinearPad:
    case StyleKind::kLinearRepeat:
    case StyleKind::kLinearReflect: {
      CGFloat reflectScale = style == StyleKind::kLinearReflect ? 1.0 : 2.0;
      CGFloat w = rect.w * reflectScale;
      CGFloat h = rect.h * reflectScale;

      CGFloat x0 = CGFloat(rect.x) + w * CGFloat(0.2);
      CGFloat y0 = CGFloat(rect.y) + h * CGFloat(0.2);
      CGFloat x1 = CGFloat(rect.x) + w * CGFloat(0.8);
      CGFloat y1 = CGFloat(rect.y) + h * CGFloat(0.8);

      CGGradientRef gradient = createGradient(style);
      CGGradientDrawingOptions options = kCGGradientDrawsBeforeStartLocation | kCGGradientDrawsAfterEndLocation;
      CGContextDrawLinearGradient(_cgContext, gradient, CGPointMake(x0, y0), CGPointMake(x1, y1), options);
      CGGradientRelease(gradient);
      break;
    }

    case StyleKind::kRadialPad:
    case StyleKind::kRadialRepeat:
    case StyleKind::kRadialReflect: {
      CGFloat cx = CGFloat(rect.x + rect.w / 2);
      CGFloat cy = CGFloat(rect.y + rect.h / 2);
      CGFloat cr = CGFloat((rect.w + rect.h) / 4);
      CGFloat fx = CGFloat(cx - cr / 2);
      CGFloat fy = CGFloat(cy - cr / 2);

      CGGradientRef gradient = createGradient(style);
      CGGradientDrawingOptions options = kCGGradientDrawsBeforeStartLocation | kCGGradientDrawsAfterEndLocation;
      CGContextDrawRadialGradient(_cgContext, gradient, CGPointMake(cx, cy), cr, CGPointMake(fx, fy), CGFloat(0.0), options);
      CGGradientRelease(gradient);
      break;
    }

    case StyleKind::kConic: {
      double cx = rect.x + rect.w / 2;
      double cy = rect.y + rect.h / 2;
      double angle = 0.0;

      CGGradientRef gradient = createGradient(style);
      CGContextDrawConicGradient(_cgContext, gradient, CGPointMake(cx, cy), CGFloat(angle));
      CGGradientRelease(gradient);
      break;
    }

    case StyleKind::kPatternNN:
    case StyleKind::kPatternBI: {
      uint32_t spriteId = nextSpriteId();

      CGContextDrawImage(_cgContext, toCGRect(rect), _cgSprites[spriteId]);
      break;
    }
  }

  if (kSaveGState)
    CGContextRestoreGState(_cgContext);
}

template<bool kSaveGState, typename RectT>
BL_INLINE void CoreGraphicsModule::renderStyledRect(const RectT& rect, StyleKind style, RenderOp op) noexcept {
  CGContextAddRect(_cgContext, toCGRect(rect));
  renderStyledPath<kSaveGState>(rect, style, op);
}

void CoreGraphicsModule::renderRectA(RenderOp op) {
  BLSizeI bounds(_params.screenW, _params.screenH);
  StyleKind style = _params.style;
  int wh = _params.shapeSize;

  if (style == StyleKind::kSolid) {
    for (uint32_t i = 0, quantity = _params.quantity; i < quantity; i++) {
      renderSolidRect(_rndCoord.nextRectI(bounds, wh, wh), op);
    }
  }
  else if ((style == StyleKind::kPatternNN || style == StyleKind::kPatternBI) && op != RenderOp::kStroke) {
    CGFloat wh_f = CGFloat(wh);
    for (uint32_t i = 0, quantity = _params.quantity; i < quantity; i++) {
      BLRectI r = _rndCoord.nextRectI(bounds, wh, wh);
      uint32_t spriteId = nextSpriteId();

      CGContextDrawImage(_cgContext, CGRectMake(CGFloat(r.x), CGFloat(r.y), wh_f, wh_f), _cgSprites[spriteId]);
    }
  }
  else {
    for (uint32_t i = 0, quantity = _params.quantity; i < quantity; i++) {
      renderStyledRect<true>(_rndCoord.nextRectI(bounds, wh, wh), style, op);
    }
  }
}

void CoreGraphicsModule::renderRectF(RenderOp op) {
  BLSize bounds(_params.screenW, _params.screenH);
  StyleKind style = _params.style;
  double wh = _params.shapeSize;

  if (style == StyleKind::kSolid) {
    for (uint32_t i = 0, quantity = _params.quantity; i < quantity; i++) {
      renderSolidRect(_rndCoord.nextRect(bounds, wh, wh), op);
    }
  }
  else {
    for (uint32_t i = 0, quantity = _params.quantity; i < quantity; i++) {
      renderStyledRect<true>(_rndCoord.nextRect(bounds, wh, wh), style, op);
    }
  }
}

void CoreGraphicsModule::renderRectRotated(RenderOp op) {
  BLSize bounds(_params.screenW, _params.screenH);
  StyleKind style = _params.style;

  double cx = double(_params.screenW) * 0.5;
  double cy = double(_params.screenH) * 0.5;
  double wh = _params.shapeSize;
  double angle = 0.0;

  for (uint32_t i = 0, quantity = _params.quantity; i < quantity; i++, angle += 0.01) {
    BLRect rect(_rndCoord.nextRect(bounds, wh, wh));

    CGContextSaveGState(_cgContext);
    CGContextTranslateCTM(_cgContext, CGFloat(cx), CGFloat(cy));
    CGContextRotateCTM(_cgContext, CGFloat(angle));
    CGContextTranslateCTM(_cgContext, CGFloat(-cx), CGFloat(-cy));

    if (style == StyleKind::kSolid) {
      renderSolidRect(rect, op);
    }
    else {
      renderStyledRect<false>(rect, style, op);
    }

    CGContextRestoreGState(_cgContext);
  }
}

void CoreGraphicsModule::renderRoundF(RenderOp op) {
  BLSize bounds(_params.screenW, _params.screenH);
  StyleKind style = _params.style;
  double wh = _params.shapeSize;

  for (uint32_t i = 0, quantity = _params.quantity; i < quantity; i++) {
    BLRect rect(_rndCoord.nextRect(bounds, wh, wh));
    double radius = _rndExtra.nextDouble(4.0, 40.0);

    CGPathRef path = CGPathCreateWithRoundedRect(
      CGRectMake(rect.x, rect.y, rect.w, rect.h),
      std::min(rect.w * 0.5, radius),
      std::min(rect.h * 0.5, radius),
      nullptr);

    CGContextAddPath(_cgContext, path);

    if (style == StyleKind::kSolid) {
      renderSolidPath(op);
    }
    else {
      renderStyledPath<true>(rect, style, op);
    }

    CGPathRelease(path);
  }
}

void CoreGraphicsModule::renderRoundRotated(RenderOp op) {
  BLSize bounds(_params.screenW, _params.screenH);
  StyleKind style = _params.style;

  double cx = double(_params.screenW) * 0.5;
  double cy = double(_params.screenH) * 0.5;
  double wh = _params.shapeSize;
  double angle = 0.0;

  for (uint32_t i = 0, quantity = _params.quantity; i < quantity; i++, angle += 0.01) {
    BLRect rect(_rndCoord.nextRect(bounds, wh, wh));
    double radius = _rndExtra.nextDouble(4.0, 40.0);

    CGContextSaveGState(_cgContext);
    CGContextTranslateCTM(_cgContext, CGFloat(cx), CGFloat(cy));
    CGContextRotateCTM(_cgContext, CGFloat(angle));
    CGContextTranslateCTM(_cgContext, CGFloat(-cx), CGFloat(-cy));

    CGPathRef path = CGPathCreateWithRoundedRect(
      CGRectMake(rect.x, rect.y, rect.w, rect.h),
      std::min(rect.w * 0.5, radius),
      std::min(rect.h * 0.5, radius),
      nullptr);

    CGContextAddPath(_cgContext, path);

    if (style == StyleKind::kSolid) {
      renderSolidPath(op);
    }
    else {
      renderStyledPath<false>(rect, style, op);
    }

    CGPathRelease(path);
    CGContextRestoreGState(_cgContext);
  }
}

void CoreGraphicsModule::renderPolygon(RenderOp op, uint32_t complexity) {
  BLSizeI bounds(_params.screenW - _params.shapeSize,
                 _params.screenH - _params.shapeSize);
  StyleKind style = _params.style;
  double wh = double(_params.shapeSize);

  for (uint32_t i = 0, quantity = _params.quantity; i < quantity; i++) {
    BLPoint base(_rndCoord.nextPoint(bounds));

    double x = _rndCoord.nextDouble(base.x, base.x + wh);
    double y = _rndCoord.nextDouble(base.y, base.y + wh);

    CGContextMoveToPoint(_cgContext, CGFloat(x), CGFloat(y));
    for (uint32_t p = 1; p < complexity; p++) {
      x = _rndCoord.nextDouble(base.x, base.x + wh);
      y = _rndCoord.nextDouble(base.y, base.y + wh);
      CGContextAddLineToPoint(_cgContext, CGFloat(x), CGFloat(y));
    }
    CGContextClosePath(_cgContext);

    if (style == StyleKind::kSolid) {
      renderSolidPath(op);
    }
    else {
      renderStyledPath<true>(BLRect(x, y, wh, wh), style, op);
    }
  }
}

void CoreGraphicsModule::renderShape(RenderOp op, ShapeData shape) {
  BLSizeI bounds(_params.screenW - _params.shapeSize,
                 _params.screenH - _params.shapeSize);
  StyleKind style = _params.style;
  double wh = double(_params.shapeSize);

  CGMutablePathRef path = CGPathCreateMutable();
  ShapeIterator it(shape);

  while (it.hasCommand()) {
    if (it.isMoveTo()) {
      CGPathMoveToPoint(
        path,
        nullptr,
        it.x(0) * wh, it.y(0) * wh);
    }
    else if (it.isLineTo()) {
      CGPathAddLineToPoint(
        path,
        nullptr,
        it.x(0) * wh, it.y(0) * wh);
    }
    else if (it.isQuadTo()) {
      CGPathAddQuadCurveToPoint(
        path,
        nullptr,
        it.x(0) * wh, it.y(0) * wh,
        it.x(1) * wh, it.y(1) * wh);
    }
    else if (it.isCubicTo()) {
      CGPathAddCurveToPoint(
        path,
        nullptr,
        it.x(0) * wh, it.y(0) * wh,
        it.x(1) * wh, it.y(1) * wh,
        it.x(2) * wh, it.y(2) * wh);
    }
    else {
      CGPathCloseSubpath(path);
    }
    it.next();
  }

  for (uint32_t i = 0, quantity = _params.quantity; i < quantity; i++) {
    BLPoint base(_rndCoord.nextPoint(bounds));

    CGContextSaveGState(_cgContext);
    CGContextTranslateCTM(_cgContext, CGFloat(base.x), CGFloat(base.y));
    CGContextAddPath(_cgContext, path);

    if (style == StyleKind::kSolid) {
      renderSolidPath(op);
    }
    else {
      renderStyledPath<false>(BLRect(base.x, base.y, wh, wh), style, op);
    }

    CGContextRestoreGState(_cgContext);
  }

  CGPathRelease(path);
}

Backend* createCGBackend() {
  return new CoreGraphicsModule();
}

} // {blbench}

#endif // BLEND2D_APPS_ENABLE_QT

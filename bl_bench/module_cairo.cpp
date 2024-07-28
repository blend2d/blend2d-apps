// This file is part of Blend2D project <https://blend2d.com>
//
// See LICENSE.md for license and copyright information
// SPDX-License-Identifier: Zlib

#ifdef BLEND2D_APPS_ENABLE_CAIRO

#include "app.h"
#include "module_cairo.h"

#include <algorithm>
#include <cairo.h>

namespace blbench {

static inline double u8ToUnit(int x) {
  static const double kDiv255 = 1.0 / 255.0;
  return double(x) * kDiv255;
}

static uint32_t toCairoFormat(uint32_t format) {
  switch (format) {
    case BL_FORMAT_PRGB32: return CAIRO_FORMAT_ARGB32;
    case BL_FORMAT_XRGB32: return CAIRO_FORMAT_RGB24;
    case BL_FORMAT_A8    : return CAIRO_FORMAT_A8;
    default:
      return 0xFFFFFFFFu;
  }
}

static uint32_t toCairoOperator(uint32_t compOp) {
  switch (compOp) {
    case BL_COMP_OP_SRC_OVER   : return CAIRO_OPERATOR_OVER;
    case BL_COMP_OP_SRC_COPY   : return CAIRO_OPERATOR_SOURCE;
    case BL_COMP_OP_SRC_IN     : return CAIRO_OPERATOR_IN;
    case BL_COMP_OP_SRC_OUT    : return CAIRO_OPERATOR_OUT;
    case BL_COMP_OP_SRC_ATOP   : return CAIRO_OPERATOR_ATOP;
    case BL_COMP_OP_DST_OVER   : return CAIRO_OPERATOR_DEST_OVER;
    case BL_COMP_OP_DST_COPY   : return CAIRO_OPERATOR_DEST;
    case BL_COMP_OP_DST_IN     : return CAIRO_OPERATOR_DEST_IN;
    case BL_COMP_OP_DST_OUT    : return CAIRO_OPERATOR_DEST_OUT;
    case BL_COMP_OP_DST_ATOP   : return CAIRO_OPERATOR_DEST_ATOP;
    case BL_COMP_OP_XOR        : return CAIRO_OPERATOR_XOR;
    case BL_COMP_OP_CLEAR      : return CAIRO_OPERATOR_CLEAR;
    case BL_COMP_OP_PLUS       : return CAIRO_OPERATOR_ADD;
    case BL_COMP_OP_MULTIPLY   : return CAIRO_OPERATOR_MULTIPLY;
    case BL_COMP_OP_SCREEN     : return CAIRO_OPERATOR_SCREEN;
    case BL_COMP_OP_OVERLAY    : return CAIRO_OPERATOR_OVERLAY;
    case BL_COMP_OP_DARKEN     : return CAIRO_OPERATOR_DARKEN;
    case BL_COMP_OP_LIGHTEN    : return CAIRO_OPERATOR_LIGHTEN;
    case BL_COMP_OP_COLOR_DODGE: return CAIRO_OPERATOR_COLOR_DODGE;
    case BL_COMP_OP_COLOR_BURN : return CAIRO_OPERATOR_COLOR_BURN;
    case BL_COMP_OP_HARD_LIGHT : return CAIRO_OPERATOR_HARD_LIGHT;
    case BL_COMP_OP_SOFT_LIGHT : return CAIRO_OPERATOR_SOFT_LIGHT;
    case BL_COMP_OP_DIFFERENCE : return CAIRO_OPERATOR_DIFFERENCE;
    case BL_COMP_OP_EXCLUSION  : return CAIRO_OPERATOR_EXCLUSION;

    default:
      return 0xFFFFFFFFu;
  }
}

static void roundRect(cairo_t* ctx, const BLRect& rect, double radius) {
  double rw2 = rect.w * 0.5;
  double rh2 = rect.h * 0.5;

  double rx = std::min(blAbs(radius), rw2);
  double ry = std::min(blAbs(radius), rh2);

  double kappaInv = 1 - 0.551915024494;
  double kx = rx * kappaInv;
  double ky = ry * kappaInv;

  double x0 = rect.x;
  double y0 = rect.y;
  double x1 = rect.x + rect.w;
  double y1 = rect.y + rect.h;

  cairo_move_to(ctx, x0 + rx, y0);
  cairo_line_to(ctx, x1 - rx, y0);
  cairo_curve_to(ctx, x1 - kx, y0, x1, y0 + ky, x1, y0 + ry);

  cairo_line_to(ctx, x1, y1 - ry);
  cairo_curve_to(ctx, x1, y1 - ky, x1 - kx, y1, x1 - rx, y1);

  cairo_line_to(ctx, x0 + rx, y1);
  cairo_curve_to(ctx, x0 + kx, y1, x0, y1 - ky, x0, y1 - ry);

  cairo_line_to(ctx, x0, y0 + ry);
  cairo_curve_to(ctx, x0, y0 + ky, x0 + kx, y0, x0 + rx, y0);

  cairo_close_path(ctx);
}

struct CairoModule : public BenchModule {
  cairo_surface_t* _cairoSurface {};
  cairo_surface_t* _cairoSprites[kBenchNumSprites] {};
  cairo_t* _cairoContext {};

  // Initialized by beforeRun().
  uint32_t _patternExtend {};
  uint32_t _patternFilter {};

  CairoModule();
  ~CairoModule() override;

  void serializeInfo(JSONBuilder& json) const override;

  template<typename RectT>
  void setupStyle(StyleKind style, const RectT& rect);

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

CairoModule::CairoModule() {
  strcpy(_name, "Cairo");
}
CairoModule::~CairoModule() {}

void CairoModule::serializeInfo(JSONBuilder& json) const {
  BLRuntimeBuildInfo buildInfo;
  BLRuntime::queryBuildInfo(&buildInfo);

  json.beforeRecord()
      .addKey("version")
      .addString(cairo_version_string());
}

template<typename RectT>
void CairoModule::setupStyle(StyleKind style, const RectT& rect) {
  switch (style) {
    case StyleKind::kSolid: {
      BLRgba32 c(_rndColor.nextRgba32());
      cairo_set_source_rgba(_cairoContext, u8ToUnit(c.r()), u8ToUnit(c.g()), u8ToUnit(c.b()), u8ToUnit(c.a()));
      return;
    }

    case StyleKind::kLinearPad:
    case StyleKind::kLinearRepeat:
    case StyleKind::kLinearReflect:
    case StyleKind::kRadialPad:
    case StyleKind::kRadialRepeat:
    case StyleKind::kRadialReflect: {
      double x = rect.x;
      double y = rect.y;

      BLRgba32 c0(_rndColor.nextRgba32());
      BLRgba32 c1(_rndColor.nextRgba32());
      BLRgba32 c2(_rndColor.nextRgba32());

      cairo_pattern_t* pattern {};
      if (style < StyleKind::kRadialPad) {
        // Linear gradient.
        double x0 = rect.x + rect.w * 0.2;
        double y0 = rect.y + rect.h * 0.2;
        double x1 = rect.x + rect.w * 0.8;
        double y1 = rect.y + rect.h * 0.8;
        pattern = cairo_pattern_create_linear(x0, y0, x1, y1);

        cairo_pattern_add_color_stop_rgba(pattern, 0.0, u8ToUnit(c0.r()), u8ToUnit(c0.g()), u8ToUnit(c0.b()), u8ToUnit(c0.a()));
        cairo_pattern_add_color_stop_rgba(pattern, 0.5, u8ToUnit(c1.r()), u8ToUnit(c1.g()), u8ToUnit(c1.b()), u8ToUnit(c1.a()));
        cairo_pattern_add_color_stop_rgba(pattern, 1.0, u8ToUnit(c2.r()), u8ToUnit(c2.g()), u8ToUnit(c2.b()), u8ToUnit(c2.a()));
      }
      else {
        // Radial gradient.
        x += double(rect.w) / 2.0;
        y += double(rect.h) / 2.0;

        double r = double(rect.w + rect.h) / 4.0;
        pattern = cairo_pattern_create_radial(x, y, r, x - r / 2, y - r / 2, 0.0);

        // Color stops in Cairo's radial gradient are reverse to Blend/Qt.
        cairo_pattern_add_color_stop_rgba(pattern, 0.0, u8ToUnit(c2.r()), u8ToUnit(c2.g()), u8ToUnit(c2.b()), u8ToUnit(c2.a()));
        cairo_pattern_add_color_stop_rgba(pattern, 0.5, u8ToUnit(c1.r()), u8ToUnit(c1.g()), u8ToUnit(c1.b()), u8ToUnit(c1.a()));
        cairo_pattern_add_color_stop_rgba(pattern, 1.0, u8ToUnit(c0.r()), u8ToUnit(c0.g()), u8ToUnit(c0.b()), u8ToUnit(c0.a()));
      }

      cairo_pattern_set_extend(pattern, cairo_extend_t(_patternExtend));
      cairo_set_source(_cairoContext, pattern);
      cairo_pattern_destroy(pattern);
      return;
    }

    case StyleKind::kPatternNN:
    case StyleKind::kPatternBI: {
      // Matrix associated with cairo_pattern_t is inverse to Blend/Qt.
      cairo_matrix_t matrix;
      cairo_matrix_init_translate(&matrix, -rect.x, -rect.y);

      cairo_pattern_t* pattern = cairo_pattern_create_for_surface(_cairoSprites[nextSpriteId()]);
      cairo_pattern_set_matrix(pattern, &matrix);
      cairo_pattern_set_extend(pattern, cairo_extend_t(_patternExtend));
      cairo_pattern_set_filter(pattern, cairo_filter_t(_patternFilter));

      cairo_set_source(_cairoContext, pattern);
      cairo_pattern_destroy(pattern);
      return;
    }

    default: {
      return;
    }
  }
}

bool CairoModule::supportsCompOp(BLCompOp compOp) const {
  return toCairoOperator(compOp) != 0xFFFFFFFFu;
}

bool CairoModule::supportsStyle(StyleKind style) const {
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

void CairoModule::beforeRun() {
  int w = int(_params.screenW);
  int h = int(_params.screenH);
  StyleKind style = _params.style;

  // Initialize the sprites.
  for (uint32_t i = 0; i < kBenchNumSprites; i++) {
    const BLImage& sprite = _sprites[i];

    BLImageData spriteData;
    sprite.getData(&spriteData);

    int stride = int(spriteData.stride);
    int format = toCairoFormat(spriteData.format);
    unsigned char* pixels = static_cast<unsigned char*>(spriteData.pixelData);

    cairo_surface_t* cairoSprite = cairo_image_surface_create_for_data(
      pixels, cairo_format_t(format), spriteData.size.w, spriteData.size.h, stride);

    _cairoSprites[i] = cairoSprite;
  }

  // Initialize the surface and the context.
  {
    BLImageData surfaceData;
    _surface.create(w, h, _params.format);
    _surface.makeMutable(&surfaceData);

    int stride = int(surfaceData.stride);
    int format = toCairoFormat(surfaceData.format);
    unsigned char* pixels = (unsigned char*)surfaceData.pixelData;

    _cairoSurface = cairo_image_surface_create_for_data(
      pixels, cairo_format_t(format), w, h, stride);

    if (_cairoSurface == nullptr)
      return;

    _cairoContext = cairo_create(_cairoSurface);
    if (_cairoContext == nullptr)
      return;
  }

  // Setup the context.
  cairo_set_operator(_cairoContext, CAIRO_OPERATOR_CLEAR);
  cairo_rectangle(_cairoContext, 0, 0, w, h);
  cairo_fill(_cairoContext);

  cairo_set_operator(_cairoContext, cairo_operator_t(toCairoOperator(_params.compOp)));
  cairo_set_line_width(_cairoContext, _params.strokeWidth);

  // Setup globals.
  _patternExtend = CAIRO_EXTEND_REPEAT;
  _patternFilter = CAIRO_FILTER_NEAREST;

  switch (style) {
    case StyleKind::kSolid          : break;
    case StyleKind::kLinearPad      : _patternExtend = CAIRO_EXTEND_PAD     ; break;
    case StyleKind::kLinearRepeat   : _patternExtend = CAIRO_EXTEND_REPEAT  ; break;
    case StyleKind::kLinearReflect  : _patternExtend = CAIRO_EXTEND_REFLECT ; break;
    case StyleKind::kRadialPad      : _patternExtend = CAIRO_EXTEND_PAD     ; break;
    case StyleKind::kRadialRepeat   : _patternExtend = CAIRO_EXTEND_REPEAT  ; break;
    case StyleKind::kRadialReflect  : _patternExtend = CAIRO_EXTEND_REFLECT ; break;
    case StyleKind::kPatternNN      : _patternFilter = CAIRO_FILTER_NEAREST ; break;
    case StyleKind::kPatternBI      : _patternFilter = CAIRO_FILTER_BILINEAR; break;

    // These are not supported.
    case StyleKind::kConic:
      break;
  }
}

void CairoModule::flush() {
  // Nothing...
}

void CairoModule::afterRun() {
  // Free the surface & the context.
  cairo_destroy(_cairoContext);
  cairo_surface_destroy(_cairoSurface);

  _cairoContext = nullptr;
  _cairoSurface = nullptr;

  // Free the sprites.
  for (uint32_t i = 0; i < kBenchNumSprites; i++) {
    cairo_surface_destroy(_cairoSprites[i]);
    _cairoSprites[i] = nullptr;
  }
}

void CairoModule::renderRectA(RenderOp op) {
  BLSizeI bounds(_params.screenW, _params.screenH);
  StyleKind style = _params.style;

  int wh = _params.shapeSize;

  for (uint32_t i = 0, quantity = _params.quantity; i < quantity; i++) {
    BLRectI rect(_rndCoord.nextRectI(bounds, wh, wh));
    setupStyle<BLRectI>(style, rect);

    if (op == RenderOp::kStroke) {
      cairo_rectangle(_cairoContext, rect.x + 0.5, rect.y + 0.5, rect.w, rect.h);
      cairo_stroke(_cairoContext);
    }
    else {
      cairo_rectangle(_cairoContext, rect.x, rect.y, rect.w, rect.h);
      cairo_fill(_cairoContext);
    }
  }
}

void CairoModule::renderRectF(RenderOp op) {
  BLSize bounds(_params.screenW, _params.screenH);
  StyleKind style = _params.style;

  double wh = _params.shapeSize;

  for (uint32_t i = 0, quantity = _params.quantity; i < quantity; i++) {
    BLRect rect(_rndCoord.nextRect(bounds, wh, wh));

    setupStyle<BLRect>(style, rect);
    cairo_rectangle(_cairoContext, rect.x, rect.y, rect.w, rect.h);

    if (op == RenderOp::kStroke)
      cairo_stroke(_cairoContext);
    else
      cairo_fill(_cairoContext);
  }
}

void CairoModule::renderRectRotated(RenderOp op) {
  BLSize bounds(_params.screenW, _params.screenH);
  StyleKind style = _params.style;

  double cx = double(_params.screenW) * 0.5;
  double cy = double(_params.screenH) * 0.5;
  double wh = _params.shapeSize;
  double angle = 0.0;

  for (uint32_t i = 0, quantity = _params.quantity; i < quantity; i++, angle += 0.01) {
    BLRect rect(_rndCoord.nextRect(bounds, wh, wh));

    cairo_translate(_cairoContext, cx, cy);
    cairo_rotate(_cairoContext, angle);
    cairo_translate(_cairoContext, -cx, -cy);

    setupStyle<BLRect>(style, rect);
    cairo_rectangle(_cairoContext, rect.x, rect.y, rect.w, rect.h);

    if (op == RenderOp::kStroke)
      cairo_stroke(_cairoContext);
    else
      cairo_fill(_cairoContext);

    cairo_identity_matrix(_cairoContext);
  }
}

void CairoModule::renderRoundF(RenderOp op) {
  BLSize bounds(_params.screenW, _params.screenH);
  StyleKind style = _params.style;

  double wh = _params.shapeSize;

  for (uint32_t i = 0, quantity = _params.quantity; i < quantity; i++) {
    BLRect rect(_rndCoord.nextRect(bounds, wh, wh));
    double radius = _rndExtra.nextDouble(4.0, 40.0);

    setupStyle<BLRect>(style, rect);
    roundRect(_cairoContext, rect, radius);

    if (op == RenderOp::kStroke)
      cairo_stroke(_cairoContext);
    else
      cairo_fill(_cairoContext);
  }
}

void CairoModule::renderRoundRotated(RenderOp op) {
  BLSize bounds(_params.screenW, _params.screenH);
  StyleKind style = _params.style;

  double cx = double(_params.screenW) * 0.5;
  double cy = double(_params.screenH) * 0.5;
  double wh = _params.shapeSize;
  double angle = 0.0;

  for (uint32_t i = 0, quantity = _params.quantity; i < quantity; i++, angle += 0.01) {
    BLRect rect(_rndCoord.nextRect(bounds, wh, wh));
    double radius = _rndExtra.nextDouble(4.0, 40.0);

    cairo_translate(_cairoContext, cx, cy);
    cairo_rotate(_cairoContext, angle);
    cairo_translate(_cairoContext, -cx, -cy);

    setupStyle<BLRect>(style, rect);
    roundRect(_cairoContext, rect, radius);

    if (op == RenderOp::kStroke)
      cairo_stroke(_cairoContext);
    else
      cairo_fill(_cairoContext);

    cairo_identity_matrix(_cairoContext);
  }
}

void CairoModule::renderPolygon(RenderOp op, uint32_t complexity) {
  BLSizeI bounds(_params.screenW - _params.shapeSize,
                 _params.screenH - _params.shapeSize);
  StyleKind style = _params.style;
  double wh = double(_params.shapeSize);

  cairo_set_fill_rule(_cairoContext, op == RenderOp::kFillEvenOdd ? CAIRO_FILL_RULE_EVEN_ODD : CAIRO_FILL_RULE_WINDING);

  for (uint32_t i = 0, quantity = _params.quantity; i < quantity; i++) {
    BLPoint base(_rndCoord.nextPoint(bounds));

    double x = _rndCoord.nextDouble(base.x, base.x + wh);
    double y = _rndCoord.nextDouble(base.y, base.y + wh);

    cairo_move_to(_cairoContext, x, y);
    for (uint32_t p = 1; p < complexity; p++) {
      x = _rndCoord.nextDouble(base.x, base.x + wh);
      y = _rndCoord.nextDouble(base.y, base.y + wh);
      cairo_line_to(_cairoContext, x, y);
    }
    setupStyle<BLRect>(style, BLRect(base.x, base.y, wh, wh));

    if (op == RenderOp::kStroke)
      cairo_stroke(_cairoContext);
    else
      cairo_fill(_cairoContext);
  }
}

void CairoModule::renderShape(RenderOp op, ShapeData shape) {
  BLSizeI bounds(_params.screenW - _params.shapeSize,
                 _params.screenH - _params.shapeSize);
  StyleKind style = _params.style;
  double wh = double(_params.shapeSize);

  ShapeIterator it(shape);
  while (it.hasCommand()) {
    if (it.isMoveTo()) {
      cairo_move_to(_cairoContext, it.x(0) * wh, it.y(0) * wh);
    }
    else if (it.isLineTo()) {
      cairo_line_to(_cairoContext, it.x(0) * wh, it.y(0) * wh);
    }
    else if (it.isQuadTo()) {
      double x0 = it.x(-1) * wh;
      double y0 = it.y(-1) * wh;
      double x1 = it.x(0) * wh;
      double y1 = it.y(0) * wh;
      double x2 = it.x(1) * wh;
      double y2 = it.y(1) * wh;

      cairo_curve_to(_cairoContext,
        (2.0 / 3.0) * x1 + (1.0 / 3.0) * x0, (2.0 / 3.0) * y1 + (1.0 / 3.0) * y0,
        (2.0 / 3.0) * x1 + (1.0 / 3.0) * x2, (2.0 / 3.0) * y1 + (1.0 / 3.0) * y2,
        y1, y2);
    }
    else if (it.isCubicTo()) {
      cairo_curve_to(_cairoContext,
        it.x(0) * wh, it.y(0) * wh,
        it.x(1) * wh, it.y(1) * wh,
        it.x(2) * wh, it.y(2) * wh);
    }
    else {
      cairo_close_path(_cairoContext);
    }
    it.next();
  }

  // No idea who invented this, but you need a `cairo_t` to create a `cairo_path_t`.
  cairo_path_t* path = cairo_copy_path(_cairoContext);
  cairo_new_path(_cairoContext);

  cairo_set_fill_rule(_cairoContext, op == RenderOp::kFillEvenOdd ? CAIRO_FILL_RULE_EVEN_ODD : CAIRO_FILL_RULE_WINDING);

  for (uint32_t i = 0, quantity = _params.quantity; i < quantity; i++) {
    cairo_save(_cairoContext);

    BLPoint base(_rndCoord.nextPoint(bounds));
    setupStyle<BLRect>(style, BLRect(base.x, base.y, wh, wh));

    cairo_translate(_cairoContext, base.x, base.y);
    cairo_append_path(_cairoContext, path);

    if (op == RenderOp::kStroke)
      cairo_stroke(_cairoContext);
    else
      cairo_fill(_cairoContext);

    cairo_restore(_cairoContext);
  }

  cairo_path_destroy(path);
}

BenchModule* createCairoModule() {
  return new CairoModule();
}
} // {blbench}

#endif // BLEND2D_APPS_ENABLE_CAIRO

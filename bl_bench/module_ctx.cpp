// This file is part of Blend2D project <https://blend2d.com>
//
// See LICENSE.md for license and copyright information
// SPDX-License-Identifier: Zlib

#ifdef BLEND2D_APPS_ENABLE_CTX

#if 0
 // if uncommented this causes us to build and include our own ctx
 // implementation we lose benefit of SIMD acceleration of solid-source-over
 // and nearest neighbor textures, there are some other slight performance loss
 // over a built library, but those seem within 10%

#include <stdint.h>
#define CTX_RASTERIZER_AA  3
#define CTX_FORCE_INLINES  1
#define CTX_PROTOCOL_U8_COLOR 1
#define CTX_IMPLEMENTATION 1
#define CTX_TERMINAL_EVENTS 0
#define CTX_FORMATTER 0
#define CTX_FAST_FILL_RECT 1
#define CTX_FAST_STROKE_RECT 1
#define CTX_FRAGMENT_SPECIALIZE 1
#define CTX_INLINED_NORMAL 1
#define CTX_DITHER 0
#define CTX_ENABLE_CM 0 
#define CTX_ENABLE_CMYK 0 
#define CTX_RASTERIZER_SWITCH_DISPATCH 1
#define CTX_EVENTS 0
#define CTX_RASTERIZER_O3 1
#endif

#include "./app.h"
#include "./module_ctx.h"

#include <algorithm>
#include <ctx.h>

namespace blbench {

static inline double u8ToUnit(int x) {
  static const double kDiv255 = 1.0 / 255.0;
  return double(x) * kDiv255;
}

static uint32_t toCtxFormat(uint32_t format) {
  switch (format) {
    case BL_FORMAT_PRGB32: return CTX_FORMAT_RGBA8;
    case BL_FORMAT_XRGB32: return CTX_FORMAT_RGBA8;
    case BL_FORMAT_A8    : return CTX_FORMAT_GRAYA8;
    default:
      return 0xFFFFFFFFu;
  }
}

static uint32_t toCtxOperator(uint32_t compOp) {
  switch (compOp) {
    case BL_COMP_OP_SRC_OVER   : return CTX_COMPOSITE_SOURCE_OVER;
    case BL_COMP_OP_SRC_COPY   : return CTX_COMPOSITE_COPY;
    case BL_COMP_OP_SRC_IN     : return CTX_COMPOSITE_CLEAR;
#if 0
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
#endif
    default:
      return 0xFFFFFFFFu;
  }
}

static void roundRect(Ctx* ctx, const BLRect& rect, double radius) {
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

  ctx_move_to(ctx, x0 + rx, y0);
  ctx_line_to(ctx, x1 - rx, y0);
  ctx_curve_to(ctx, x1 - kx, y0, x1, y0 + ky, x1, y0 + ry);

  ctx_line_to(ctx, x1, y1 - ry);
  ctx_curve_to(ctx, x1, y1 - ky, x1 - kx, y1, x1 - rx, y1);

  ctx_line_to(ctx, x0 + rx, y1);
  ctx_curve_to(ctx, x0 + kx, y1, x0, y1 - ky, x0, y1 - ry);

  ctx_line_to(ctx, x0, y0 + ry);
  ctx_curve_to(ctx, x0, y0 + ky, x0 + kx, y0, x0 + rx, y0);

  ctx_close_path(ctx);
}

struct CtxModule : public BenchModule {
  //ctx_surface_t* _ctxSurface;
  //ctx_surface_t* _ctxSprites[kBenchNumSprites];
  Ctx* _ctxContext;

  // Initialized by onBeforeRun().
  uint32_t _patternExtend;
  uint32_t _patternFilter;

  CtxModule();
  virtual ~CtxModule();

  template<typename RectT>
  void setupStyle(uint32_t style, const RectT& rect);

  virtual bool supportsCompOp(BLCompOp compOp) const;
  virtual bool supportsStyle(uint32_t style) const;

  virtual void onBeforeRun();
  virtual void onAfterRun();

  virtual void onDoRectAligned(bool stroke);
  virtual void onDoRectSmooth(bool stroke);
  virtual void onDoRectRotated(bool stroke);
  virtual void onDoRoundSmooth(bool stroke);
  virtual void onDoRoundRotated(bool stroke);
  virtual void onDoPolygon(uint32_t mode, uint32_t complexity);
  virtual void onDoShape(bool stroke, const BLPoint* pts, size_t count);
};

CtxModule::CtxModule() {
  strcpy(_name, "Ctx");
  //_ctxSurface = NULL;
  _ctxContext = NULL;
  //memset(_ctxSprites, 0, sizeof(_ctxSprites));
}
CtxModule::~CtxModule() {}

template<typename RectT>
void CtxModule::setupStyle(uint32_t style, const RectT& rect) {
  switch (style) {
    case kBenchStyleSolid: {
      BLRgba32 c(_rndColor.nextRgba32());
      ctx_rgba(_ctxContext, u8ToUnit(c.r()), u8ToUnit(c.g()), u8ToUnit(c.b()), u8ToUnit(c.a()));
      return;
    }

    case kBenchStyleLinearPad:
    case kBenchStyleLinearRepeat:
    case kBenchStyleLinearReflect:
    case kBenchStyleRadialPad:
    case kBenchStyleRadialRepeat:
    case kBenchStyleRadialReflect: {
      double x = rect.x;
      double y = rect.y;

      BLRgba32 c0(_rndColor.nextRgba32());
      BLRgba32 c1(_rndColor.nextRgba32());
      BLRgba32 c2(_rndColor.nextRgba32());

      //ctx_pattern_t* pattern = NULL;
     if (style < kBenchStyleRadialPad) {
        // Linear gradient.
        double x0 = rect.x + rect.w * 0.2;
        double y0 = rect.y + rect.h * 0.2;
        double x1 = rect.x + rect.w * 0.8;
        double y1 = rect.y + rect.h * 0.8;
        ctx_linear_gradient (_ctxContext, x0, y0, x1, y1);

        ctx_gradient_add_stop (_ctxContext, 0.0, u8ToUnit(c0.r()), u8ToUnit(c0.g()), u8ToUnit(c0.b()), u8ToUnit(c0.a()));
        ctx_gradient_add_stop (_ctxContext, 0.5, u8ToUnit(c1.r()), u8ToUnit(c1.g()), u8ToUnit(c1.b()), u8ToUnit(c1.a()));
        ctx_gradient_add_stop (_ctxContext, 1.0, u8ToUnit(c2.r()), u8ToUnit(c2.g()), u8ToUnit(c2.b()), u8ToUnit(c2.a()));
      }
       else {
        // Radial gradient.
        ctx_gradient_add_stop(_ctxContext, 0.0, u8ToUnit(c2.r()), u8ToUnit(c2.g()), u8ToUnit(c2.b()), u8ToUnit(c2.a()));
        ctx_gradient_add_stop(_ctxContext, 0.5, u8ToUnit(c1.r()), u8ToUnit(c1.g()), u8ToUnit(c1.b()), u8ToUnit(c1.a()));
        ctx_gradient_add_stop(_ctxContext, 1.0, u8ToUnit(c0.r()), u8ToUnit(c0.g()), u8ToUnit(c0.b()), u8ToUnit(c0.a()));
        x += double(rect.w) / 2.0;
        y += double(rect.h) / 2.0;

        double r = double(rect.w + rect.h) / 4.0;
        ctx_radial_gradient (_ctxContext, x, y, r, x - r / 2, y - r / 2, 0.0);

        // Color stops in Ctx's radial gradient are reverse to Blend/Qt.
        ctx_gradient_add_stop(_ctxContext, 0.0, u8ToUnit(c2.r()), u8ToUnit(c2.g()), u8ToUnit(c2.b()), u8ToUnit(c2.a()));
        ctx_gradient_add_stop(_ctxContext, 0.5, u8ToUnit(c1.r()), u8ToUnit(c1.g()), u8ToUnit(c1.b()), u8ToUnit(c1.a()));
        ctx_gradient_add_stop(_ctxContext, 1.0, u8ToUnit(c0.r()), u8ToUnit(c0.g()), u8ToUnit(c0.b()), u8ToUnit(c0.a()));
      }

      //ctx_extend(_ctxContext, CTX_EXTEND_NONE);

      //ctx_pattern_set_extend(pattern, ctx_extend_t(_patternExtend));
      //ctx_set_source(_ctxContext, pattern);
      //ctx_pattern_destroy(pattern);
      return;
    }

    case kBenchStylePatternNN:
    case kBenchStylePatternBI: {
      int id = nextSpriteId();
      const BLImage& sprite = _sprites[id];
      BLImageData spriteData;
      sprite.getData(&spriteData);
      int stride = int(spriteData.stride);
      int format = toCtxFormat(spriteData.format);
      unsigned char* pixels = static_cast<unsigned char*>(spriteData.pixelData);

      char eid[64];
      sprintf (eid, "texture-%i", id);
      ctx_extend(_ctxContext, CTX_EXTEND_NONE);
      ctx_define_texture (_ctxContext, eid,
         spriteData.size.w, spriteData.size.h, stride,
         format, pixels, NULL);
      CtxMatrix mat;
      ctx_matrix_identity (&mat);
      ctx_matrix_translate (&mat, rect.x, rect.y);
      ctx_source_transform_matrix (_ctxContext, &mat);

      if (style == kBenchStylePatternNN)
        ctx_image_smoothing (_ctxContext, 0);
      else
        ctx_image_smoothing (_ctxContext, 1);
      return;
    }
  }
}

bool CtxModule::supportsCompOp(BLCompOp compOp) const {
  return toCtxOperator(compOp) != 0xFFFFFFFFu;
}

bool CtxModule::supportsStyle(uint32_t style) const {
  return style == kBenchStyleSolid         ||
         style == kBenchStyleLinearPad     ||
         style == kBenchStyleLinearRepeat  ||
         style == kBenchStyleLinearReflect ||
         style == kBenchStyleRadialPad     ||
         style == kBenchStyleRadialRepeat  ||
         style == kBenchStyleRadialReflect ||
         style == kBenchStylePatternNN     ||
         style == kBenchStylePatternBI     ;
}

void CtxModule::onBeforeRun() {
  int w = int(_params.screenW);
  int h = int(_params.screenH);
  uint32_t style = _params.style;


  // Initialize the surface and the context.
  {
    BLImageData surfaceData;
    _surface.create(w, h, _params.format);
    _surface.makeMutable(&surfaceData);

    int stride = int(surfaceData.stride);
    int format = toCtxFormat(surfaceData.format);
    unsigned char* pixels = (unsigned char*)surfaceData.pixelData;

    memset (pixels, 0, h * stride);
    _ctxContext = ctx_new_for_framebuffer (pixels, w, h, stride, CtxPixelFormat(format));

    if (_ctxContext == NULL)
      return;
  }
  // Initialize the sprites.
  for (uint32_t i = 0; i < kBenchNumSprites; i++) {
    const BLImage& sprite = _sprites[i];

    BLImageData spriteData;
    sprite.getData(&spriteData);

    int stride = int(spriteData.stride);
    int format = toCtxFormat(spriteData.format);
    unsigned char* pixels = static_cast<unsigned char*>(spriteData.pixelData);

    char *eid = (char*)malloc (40);
    sprintf (eid, "texture-%i", i);
    ctx_define_texture (_ctxContext, eid,
       spriteData.size.w, spriteData.size.h, stride,
       format,
       pixels,
       NULL);
    free (eid);

    //_ctxSprites[i] = ctxSprite;
  }

  // Setup the context.
  ctx_compositing_mode(_ctxContext, CTX_COMPOSITE_CLEAR);
  ctx_paint(_ctxContext);

  ctx_compositing_mode(_ctxContext, CtxCompositingMode(toCtxOperator(_params.compOp)));
  ctx_line_width(_ctxContext, _params.strokeWidth);

  // Setup globals.
  _patternExtend = CTX_EXTEND_REPEAT;
#if 0
  _patternFilter = CTX_FILTER_NEAREST;

  switch (style) {
    case kBenchStyleLinearPad      : _patternExtend = CAIRO_EXTEND_PAD     ; break;
    case kBenchStyleLinearRepeat   : _patternExtend = CAIRO_EXTEND_REPEAT  ; break;
    case kBenchStyleLinearReflect  : _patternExtend = CAIRO_EXTEND_REFLECT ; break;
    case kBenchStyleRadialPad      : _patternExtend = CAIRO_EXTEND_PAD     ; break;
    case kBenchStyleRadialRepeat   : _patternExtend = CAIRO_EXTEND_REPEAT  ; break;
    case kBenchStyleRadialReflect  : _patternExtend = CAIRO_EXTEND_REFLECT ; break;
    case kBenchStylePatternNN      : _patternFilter = CAIRO_FILTER_NEAREST ; break;
    case kBenchStylePatternBI      : _patternFilter = CAIRO_FILTER_BILINEAR; break;
  }
#endif
}

void CtxModule::onAfterRun() {
  // Free the surface & the context.
  ctx_destroy(_ctxContext);
  //ctx_surface_destroy(_ctxSurface);

  _ctxContext = NULL;
  //_ctxSurface = NULL;
#if 0
  // Free the sprites.
  for (uint32_t i = 0; i < kBenchNumSprites; i++) {
    ctx_surface_destroy(_ctxSprites[i]);
    _ctxSprites[i] = NULL;
  }
#endif
}

void CtxModule::onDoRectAligned(bool stroke) {
  BLSizeI bounds(_params.screenW, _params.screenH);
  uint32_t style = _params.style;

  int wh = _params.shapeSize;

  for (uint32_t i = 0, quantity = _params.quantity; i < quantity; i++) {
    BLRectI rect(_rndCoord.nextRectI(bounds, wh, wh));
    setupStyle<BLRectI>(style, rect);

    ctx_rectangle(_ctxContext, rect.x, rect.y, rect.w, rect.h);
    if (stroke)
      ctx_stroke(_ctxContext);
    else 
      ctx_fill(_ctxContext);
  }
}

void CtxModule::onDoRectSmooth(bool stroke) {
  BLSize bounds(_params.screenW, _params.screenH);
  uint32_t style = _params.style;

  double wh = _params.shapeSize;

  for (uint32_t i = 0, quantity = _params.quantity; i < quantity; i++) {
    BLRect rect(_rndCoord.nextRect(bounds, wh, wh));

    setupStyle<BLRect>(style, rect);
    ctx_rectangle(_ctxContext, rect.x, rect.y, rect.w, rect.h);

    if (stroke)
      ctx_stroke(_ctxContext);
    else
      ctx_fill(_ctxContext);
  }
}

void CtxModule::onDoRectRotated(bool stroke) {
  BLSize bounds(_params.screenW, _params.screenH);
  uint32_t style = _params.style;

  double cx = double(_params.screenW) * 0.5;
  double cy = double(_params.screenH) * 0.5;
  double wh = _params.shapeSize;
  double angle = 0.0;

  for (uint32_t i = 0, quantity = _params.quantity; i < quantity; i++, angle += 0.01) {
    BLRect rect(_rndCoord.nextRect(bounds, wh, wh));

    ctx_translate(_ctxContext, cx, cy);
    ctx_rotate(_ctxContext, angle);
    ctx_translate(_ctxContext, -cx, -cy);

    setupStyle<BLRect>(style, rect);
    ctx_rectangle(_ctxContext, rect.x, rect.y, rect.w, rect.h);

    if (stroke)
      ctx_stroke(_ctxContext);
    else
      ctx_fill(_ctxContext);

    ctx_identity(_ctxContext);
  }
}

void CtxModule::onDoRoundSmooth(bool stroke) {
  BLSize bounds(_params.screenW, _params.screenH);
  uint32_t style = _params.style;

  double wh = _params.shapeSize;

  for (uint32_t i = 0, quantity = _params.quantity; i < quantity; i++) {
    BLRect rect(_rndCoord.nextRect(bounds, wh, wh));
    double radius = _rndExtra.nextDouble(4.0, 40.0);

    setupStyle<BLRect>(style, rect);
    roundRect(_ctxContext, rect, radius);

    if (stroke)
      ctx_stroke(_ctxContext);
    else
      ctx_fill(_ctxContext);
  }
}

void CtxModule::onDoRoundRotated(bool stroke) {
  BLSize bounds(_params.screenW, _params.screenH);
  uint32_t style = _params.style;

  double cx = double(_params.screenW) * 0.5;
  double cy = double(_params.screenH) * 0.5;
  double wh = _params.shapeSize;
  double angle = 0.0;

  for (uint32_t i = 0, quantity = _params.quantity; i < quantity; i++, angle += 0.01) {
    BLRect rect(_rndCoord.nextRect(bounds, wh, wh));
    double radius = _rndExtra.nextDouble(4.0, 40.0);

    ctx_translate(_ctxContext, cx, cy);
    ctx_rotate(_ctxContext, angle);
    ctx_translate(_ctxContext, -cx, -cy);

    setupStyle<BLRect>(style, rect);
    roundRect(_ctxContext, rect, radius);

    if (stroke)
      ctx_stroke(_ctxContext);
    else
      ctx_fill(_ctxContext);

    ctx_identity(_ctxContext);
  }
}

void CtxModule::onDoPolygon(uint32_t mode, uint32_t complexity) {
  BLSizeI bounds(_params.screenW - _params.shapeSize,
                 _params.screenH - _params.shapeSize);
  uint32_t style = _params.style;
  bool stroke = (mode == 2);

  double wh = double(_params.shapeSize);

  if (mode == 0) ctx_fill_rule(_ctxContext, CTX_FILL_RULE_WINDING);
  if (mode == 1) ctx_fill_rule(_ctxContext, CTX_FILL_RULE_EVEN_ODD);

  for (uint32_t i = 0, quantity = _params.quantity; i < quantity; i++) {
    BLPoint base(_rndCoord.nextPoint(bounds));

    double x = _rndCoord.nextDouble(base.x, base.x + wh);
    double y = _rndCoord.nextDouble(base.y, base.y + wh);

    ctx_move_to(_ctxContext, x, y);
    for (uint32_t p = 1; p < complexity; p++) {
      x = _rndCoord.nextDouble(base.x, base.x + wh);
      y = _rndCoord.nextDouble(base.y, base.y + wh);
      ctx_line_to(_ctxContext, x, y);
    }
    setupStyle<BLRect>(style, BLRect(base.x, base.y, wh, wh));

    if (stroke)
      ctx_stroke(_ctxContext);
    else
      ctx_fill(_ctxContext);
  }
}


void CtxModule::onDoShape(bool stroke, const BLPoint* pts, size_t count) {
  BLSizeI bounds(_params.screenW - _params.shapeSize,
                 _params.screenH - _params.shapeSize);
  uint32_t style = _params.style;


  float wh =float(_params.shapeSize);

  // this mechanism is not only for paths..
  // it also applies to 
  Ctx *drawlist = ctx_new_drawlist(-1,-1);
  bool start = true;
  for (size_t i = 0; i < count; i++) {
    float x = pts[i].x;
    float y = pts[i].y;

    if (x == -1.0 && y == -1.0) {
      start = true;
      continue;
    }

    if (start) {
      ctx_move_to(drawlist, x * wh, y * wh);
      start = false;
    }
    else {
      ctx_line_to(drawlist, x * wh, y * wh);
    }
  }

  for (uint32_t i = 0, quantity = _params.quantity; i < quantity; i++) {
    ctx_save(_ctxContext);

    BLPoint base(_rndCoord.nextPoint(bounds));
    setupStyle<BLRect>(style, BLRect(base.x, base.y, wh, wh));
    
    ctx_translate(_ctxContext, base.x, base.y);
    ctx_render_ctx(drawlist, _ctxContext);

    if (stroke)
      ctx_stroke(_ctxContext);
    else
      ctx_fill(_ctxContext);

    ctx_restore(_ctxContext);
 }
 ctx_destroy(drawlist);
}

BenchModule* createCtxModule() {
  return new CtxModule();
}
} // {blbench}

#endif // BLEND2D_APPS_ENABLE_CAIRO

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

static inline uint32_t to_agg2d_blend_mode(BLCompOp comp_op) {
  switch (comp_op) {
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

static inline Agg2D::Color to_agg2d_color(BLRgba32 rgba32) {
  return Agg2D::Color(rgba32.r(), rgba32.g(), rgba32.b(), rgba32.a());
}

struct AggModule : public Backend {
  Agg2D _ctx;

  AggModule();
  ~AggModule() override;

  bool supports_comp_op(BLCompOp comp_op) const override;
  bool supports_style(StyleKind style) const override;

  void before_run() override;
  void flush() override;
  void after_run() override;

  void prepare_fill_stroke_option(RenderOp op);

  template<typename RectT>
  void setup_style(RenderOp op, const RectT& rect);

  void render_rect_a(RenderOp op) override;
  void render_rect_f(RenderOp op) override;
  void render_rect_rotated(RenderOp op) override;
  void render_round_f(RenderOp op) override;
  void render_round_rotated(RenderOp op) override;
  void render_polygon(RenderOp op, uint32_t complexity) override;
  void render_shape(RenderOp op, ShapeData shape) override;
};

AggModule::AggModule() {
  strcpy(_name, "AGG");
}
AggModule::~AggModule() {}

bool AggModule::supports_comp_op(BLCompOp comp_op) const {
  return to_agg2d_blend_mode(comp_op) != 0xFFFFFFFFu;
}

bool AggModule::supports_style(StyleKind style) const {
  return style == StyleKind::kSolid         ||
         style == StyleKind::kLinearPad     ||
         style == StyleKind::kLinearRepeat  ||
         style == StyleKind::kLinearReflect ||
         style == StyleKind::kRadialPad     ||
         style == StyleKind::kRadialRepeat  ||
         style == StyleKind::kRadialReflect ;
}

void AggModule::before_run() {
  int w = int(_params.screen_w);
  int h = int(_params.screen_h);

  BLImageData surface_data;
  _surface.create(w, h, BL_FORMAT_PRGB32);
  _surface.make_mutable(&surface_data);

  _ctx.attach(
    static_cast<unsigned char*>(surface_data.pixel_data),
    unsigned(surface_data.size.w),
    unsigned(surface_data.size.h),
    int(surface_data.stride));

  _ctx.fillEvenOdd(false);
  _ctx.noLine();
  _ctx.blendMode(Agg2D::BlendSrc);
  _ctx.clearAll(Agg2D::Color(0, 0, 0, 0));
  _ctx.blendMode(Agg2D::BlendMode(to_agg2d_blend_mode(_params.comp_op)));
}

void AggModule::flush() {
  // Nothing...
}

void AggModule::after_run() {
  _ctx.attach(nullptr, 0, 0, 0);
}

void AggModule::prepare_fill_stroke_option(RenderOp op) {
  if (op == RenderOp::kStroke)
    _ctx.noFill();
  else
    _ctx.noLine();
}

template<typename RectT>
void AggModule::setup_style(RenderOp op, const RectT& rect) {
  switch (_params.style) {
    case StyleKind::kSolid: {
      BLRgba32 color = _rnd_color.next_rgba32();
      if (op == RenderOp::kStroke)
        _ctx.lineColor(to_agg2d_color(color));
      else
        _ctx.fillColor(to_agg2d_color(color));
      break;
    }

    case StyleKind::kLinearPad:
    case StyleKind::kLinearRepeat:
    case StyleKind::kLinearReflect: {
      double x1 = rect.x + rect.w * 0.2;
      double y1 = rect.y + rect.h * 0.2;
      double x2 = rect.x + rect.w * 0.8;
      double y2 = rect.y + rect.h * 0.8;

      BLRgba32 c1 = _rnd_color.next_rgba32();
      BLRgba32 c2 = _rnd_color.next_rgba32();
      BLRgba32 c3 = _rnd_color.next_rgba32();

      if (op == RenderOp::kStroke)
        _ctx.lineLinearGradient(x1, y1, x2, y2, to_agg2d_color(c1), to_agg2d_color(c2), to_agg2d_color(c3));
      else
        _ctx.fillLinearGradient(x1, y1, x2, y2, to_agg2d_color(c1), to_agg2d_color(c2), to_agg2d_color(c3));
      break;
    }

    case StyleKind::kRadialPad:
    case StyleKind::kRadialRepeat:
    case StyleKind::kRadialReflect: {
      double cx = rect.x + rect.w / 2.0;
      double cy = rect.y + rect.h / 2.0;
      double cr = (rect.w + rect.h) / 4.0;

      BLRgba32 c1 = _rnd_color.next_rgba32();
      BLRgba32 c2 = _rnd_color.next_rgba32();
      BLRgba32 c3 = _rnd_color.next_rgba32();

      if (op == RenderOp::kStroke)
        _ctx.lineRadialGradient(cx, cy, cr, to_agg2d_color(c1), to_agg2d_color(c2), to_agg2d_color(c3));
      else
        _ctx.fillRadialGradient(cx, cy, cr, to_agg2d_color(c1), to_agg2d_color(c2), to_agg2d_color(c3));
      break;
    }

    default:
      break;
  }
}

void AggModule::render_rect_a(RenderOp op) {
  BLSizeI bounds(_params.screen_w, _params.screen_h);
  StyleKind style = _params.style;
  int wh = _params.shape_size;

  prepare_fill_stroke_option(op);

  if (_params.style == StyleKind::kSolid && op != RenderOp::kStroke) {
    for (uint32_t i = 0, quantity = _params.quantity; i < quantity; i++) {
      BLRectI rect = _rnd_coord.next_rect_i(bounds, wh, wh);
      _ctx.fillRectangleI(rect.x, rect.y, rect.x + rect.w, rect.y + rect.h, to_agg2d_color(_rnd_color.next_rgba32()));
    }
  }
  else {
    for (uint32_t i = 0, quantity = _params.quantity; i < quantity; i++) {
      BLRectI rect = _rnd_coord.next_rect_i(bounds, wh, wh);
      setup_style(op, rect);
      _ctx.rectangle(rect.x, rect.y, rect.x + rect.w, rect.y + rect.h);
    }
  }
}

void AggModule::render_rect_f(RenderOp op) {
  BLSize bounds(_params.screen_w, _params.screen_h);
  StyleKind style = _params.style;
  double wh = _params.shape_size;

  prepare_fill_stroke_option(op);

  for (uint32_t i = 0, quantity = _params.quantity; i < quantity; i++) {
    BLRect rect = _rnd_coord.next_rect(bounds, wh, wh);
    setup_style(op, rect);
    _ctx.rectangle(rect.x, rect.y, rect.x + rect.w, rect.y + rect.h);
  }
}

void AggModule::render_rect_rotated(RenderOp op) {
  BLSize bounds(_params.screen_w, _params.screen_h);
  StyleKind style = _params.style;

  double cx = double(_params.screen_w) * 0.5;
  double cy = double(_params.screen_h) * 0.5;
  double wh = _params.shape_size;
  double angle = 0.0;

  prepare_fill_stroke_option(op);

  for (uint32_t i = 0, quantity = _params.quantity; i < quantity; i++, angle += 0.01) {
    BLRect rect(_rnd_coord.next_rect(bounds, wh, wh));

    agg::trans_affine affine;
    affine.translate(-cx, -cy);
    affine.rotate(angle);
    affine.translate(cx, cy);

    _ctx.affine(affine);
    setup_style(op, rect);
    _ctx.rectangle(rect.x, rect.y, rect.x + rect.w, rect.y + rect.h);
    _ctx.resetTransformations();
  }
}

void AggModule::render_round_f(RenderOp op) {
  BLSize bounds(_params.screen_w, _params.screen_h);
  StyleKind style = _params.style;
  double wh = _params.shape_size;

  prepare_fill_stroke_option(op);

  for (uint32_t i = 0, quantity = _params.quantity; i < quantity; i++) {
    BLRect rect = _rnd_coord.next_rect(bounds, wh, wh);
    double radius = _rnd_extra.next_double(4.0, 40.0);

    setup_style(op, rect);
    _ctx.roundedRect(rect.x, rect.y, rect.x + rect.w, rect.y + rect.h, radius);
  }
}

void AggModule::render_round_rotated(RenderOp op) {
  BLSize bounds(_params.screen_w, _params.screen_h);
  StyleKind style = _params.style;

  double cx = double(_params.screen_w) * 0.5;
  double cy = double(_params.screen_h) * 0.5;
  double wh = _params.shape_size;
  double angle = 0.0;

  prepare_fill_stroke_option(op);

  for (uint32_t i = 0, quantity = _params.quantity; i < quantity; i++, angle += 0.01) {
    BLRect rect(_rnd_coord.next_rect(bounds, wh, wh));
    double radius = _rnd_extra.next_double(4.0, 40.0);

    agg::trans_affine affine;
    affine.translate(-cx, -cy);
    affine.rotate(angle);
    affine.translate(cx, cy);
    _ctx.affine(affine);

    setup_style(op, rect);
    _ctx.roundedRect(rect.x, rect.y, rect.x + rect.w, rect.y + rect.h, radius);
    _ctx.resetTransformations();
  }
}

void AggModule::render_polygon(RenderOp op, uint32_t complexity) {
  BLSizeI bounds(_params.screen_w - _params.shape_size,
                 _params.screen_h - _params.shape_size);

  StyleKind style = _params.style;
  double wh = _params.shape_size;

  prepare_fill_stroke_option(op);
  _ctx.fillEvenOdd(op == RenderOp::kFillEvenOdd);

  Agg2D::DrawPathFlag draw_path_flag = op == RenderOp::kStroke ? Agg2D::StrokeOnly : Agg2D::FillOnly;

  for (uint32_t i = 0, quantity = _params.quantity; i < quantity; i++) {
    BLPoint base(_rnd_coord.nextPoint(bounds));

    double x = _rnd_coord.next_double(base.x, base.x + wh);
    double y = _rnd_coord.next_double(base.y, base.y + wh);

    _ctx.resetPath();
    _ctx.moveTo(x, y);

    for (uint32_t p = 1; p < complexity; p++) {
      x = _rnd_coord.next_double(base.x, base.x + wh);
      y = _rnd_coord.next_double(base.y, base.y + wh);
      _ctx.lineTo(x, y);
    }

    setup_style(op, BLRect(base.x, base.y, wh, wh));
    _ctx.drawPath(draw_path_flag);
  }
}

void AggModule::render_shape(RenderOp op, ShapeData shape) {
  BLSizeI bounds(_params.screen_w - _params.shape_size,
                 _params.screen_h - _params.shape_size);

  StyleKind style = _params.style;
  double wh = double(_params.shape_size);

  prepare_fill_stroke_option(op);
  _ctx.fillEvenOdd(op == RenderOp::kFillEvenOdd);

  Agg2D::DrawPathFlag draw_path_flag = op == RenderOp::kStroke ? Agg2D::StrokeOnly : Agg2D::FillOnly;

  for (uint32_t i = 0, quantity = _params.quantity; i < quantity; i++) {
    BLPoint base(_rnd_coord.nextPoint(bounds));
    ShapeIterator it(shape);

    _ctx.resetPath();
    while (it.has_command()) {
      if (it.is_move_to()) {
        _ctx.moveTo(base.x + it.x(0) * wh, base.y + it.y(0) * wh);
      }
      else if (it.is_line_to()) {
        _ctx.lineTo(base.x + it.x(0) * wh, base.y + it.y(0) * wh);
      }
      else if (it.is_quad_to()) {
        _ctx.quadricCurveTo(
          base.x + it.x(0) * wh, base.y + it.y(0) * wh,
          base.x + it.x(1) * wh, base.y + it.y(1) * wh
        );
      }
      else if (it.is_cubic_to()) {
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

    setup_style(op, BLRect(base.x, base.y, wh, wh));
    _ctx.drawPath(draw_path_flag);
  }
}

Backend* create_agg_backend() {
  return new AggModule();
}

} // {blbench}

#endif // BLEND2D_APPS_ENABLE_AGG

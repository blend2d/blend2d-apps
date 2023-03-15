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

#ifdef BLEND2D_APPS_ENABLE_AGG

#include "./app.h"
#include "./module_agg.h"

#include <algorithm>
#include "agg_conv_transform.h"
#include "agg_conv_stroke.h"
#include "agg_rounded_rect.h"
#include "agg_trans_affine.h"

namespace blbench {

// ============================================================================
// [bench::AGGRectSource]
// ============================================================================

class AGGRectSource {
public:
  BLBox _box;
  size_t _index;

  inline AGGRectSource(const BLRect rect) {
    _box.x0 = rect.x;
    _box.y0 = rect.y;
    _box.x1 = rect.x + rect.w;
    _box.y1 = rect.y + rect.h;
    _index = 0;
  }

  inline void rewind(unsigned) { _index = 0; }
  inline unsigned vertex(double* x, double* y) {
    switch (_index) {
      case 0:
        *x = _box.x0;
        *y = _box.y0;
        _index++;
        return agg::path_cmd_move_to;
      case 1:
        *x = _box.x1;
        *y = _box.y0;
        _index++;
        return agg::path_cmd_line_to;
      case 2:
        *x = _box.x1;
        *y = _box.y1;
        _index++;
        return agg::path_cmd_line_to;
      case 3:
        *x = _box.x0;
        *y = _box.y1;
        _index++;
        return agg::path_cmd_line_to;

      case 4:
        _index++;
        return agg::path_cmd_end_poly | agg::path_flags_close;

      default:
        return agg::path_cmd_stop;
    }
  }
};

// ============================================================================
// [bench::AGGRandomPolySource]
// ============================================================================

class AGGRandomPolySource {
public:
  BLPoint _tr;
  double _wh;
  BenchRandom& _rnd;
  size_t _remaining;
  size_t _remainingSaved;
  agg::path_commands_e _cmd;

  inline AGGRandomPolySource(const BLPoint& tr, double wh, BenchRandom& rnd, size_t count)
    : _tr(tr),
      _wh(wh),
      _rnd(rnd),
      _remaining(count + 1),
      _remainingSaved(count),
      _cmd(agg::path_cmd_move_to) {}

  inline void rewind(unsigned) { _remaining = _remainingSaved + 1; }
  inline unsigned vertex(double* x, double* y) {
    if (_remaining <= 1) {
      if (_remaining == 0)
        return agg::path_cmd_stop;
      _remaining--;
      return agg::path_cmd_end_poly | agg::path_flags_close;
    }

    agg::path_commands_e cmd = _cmd;
    *x = _rnd.nextDouble(_tr.x, _tr.x + _wh);
    *y = _rnd.nextDouble(_tr.y, _tr.y + _wh);

    _cmd = agg::path_cmd_line_to;
    _remaining--;
    return cmd;
  }
};

// ============================================================================
// [bench::AGGShapeDataSource]
// ============================================================================

class AGGShapeDataSource {
public:
  const BLPoint* _data;
  size_t _size;
  size_t _index;
  double _scale;
  BLPoint _tr;
  agg::path_commands_e _cmd;

  inline AGGShapeDataSource(const BLPoint* data, size_t size, const BLPoint& tr, double scale) noexcept {
    _data = data;
    _size = size;
    _index = 0;
    _tr = tr;
    _scale = scale;
    _cmd = agg::path_cmd_move_to;
  }

  inline void rewind(unsigned) {}
  inline unsigned vertex(double* x, double* y) {
    if (_index >= _size)
      return agg::path_cmd_stop;

    agg::path_commands_e cmd = _cmd;
    if (_data[_index].x == -1.0) {
      cmd = agg::path_cmd_end_poly;
      _cmd = agg::path_cmd_move_to;
    }
    else {
      _cmd = agg::path_cmd_line_to;
      *x = _data[_index].x * _scale + _tr.x;
      *y = _data[_index].y * _scale + _tr.y;
    }

    _index++;
    return cmd;
  }
};

// ============================================================================
// [bench::AGGModule - Construction / Destruction]
// ============================================================================

AGGModule::AGGModule() {
  strcpy(_name, "AGG");
}
AGGModule::~AGGModule() {}

// ============================================================================
// [bench::AGGModule - AGG]
// ============================================================================

void AGGModule::renderScanlines(const BLRect& rect, uint32_t style) {
  switch (style) {
    case kBenchStyleSolid: {
      BLRgba32 c(_rndColor.nextRgba32());
      agg::rgba8 color(uint8_t(c.r()), uint8_t(c.g()), uint8_t(c.b()), uint8_t(c.a()));
      color.premultiply();
      _rendererSolid.color(color);
      agg::render_scanlines(_rasterizer, _scanline, _rendererSolid);
      break;
    }
  }

  _rasterizer.reset();
}

void AGGModule::fillRectAA(int x, int y, int w, int h, uint32_t style) {
  switch (style) {
    case kBenchStyleSolid: {
      BLRgba32 c(_rndColor.nextRgba32());
      agg::rgba8 color(uint8_t(c.r()), uint8_t(c.g()), uint8_t(c.b()), uint8_t(c.a()));
      color.premultiply();
      _rendererBase.blend_bar(x, y, x + w - 1, y + h - 1, color, 0xFFu);
      break;
    }
  }
}

template<typename T>
void AGGModule::rasterizePath(T& path, bool stroke) {
  if (stroke) {
    agg::conv_stroke<T> strokedPath(path);
    strokedPath.width(_params.strokeWidth);
    _rasterizer.add_path(strokedPath);
  }
  else {
    _rasterizer.add_path(path);
  }
}

// ============================================================================
// [bench::AGGModule - Interface]
// ============================================================================

bool AGGModule::supportsCompOp(uint32_t compOp) const {
  // TODO: SRC_COPY not supported yet.
  return /*compOp == BL_COMP_OP_SRC_COPY ||*/
         compOp == BL_COMP_OP_SRC_OVER ;
}

bool AGGModule::supportsStyle(uint32_t style) const {
  return style == kBenchStyleSolid;
}

void AGGModule::onBeforeRun() {
  int w = int(_params.screenW);
  int h = int(_params.screenH);
  uint32_t style = _params.style;

  // Initialize the sprites.

  // TODO: Not supported yet,
  /*
  for (uint32_t i = 0; i < kBenchNumSprites; i++) {
    const BLImage& sprite = _sprites[i];

    BLImageData spriteData;
    sprite.getData(&spriteData);

    int stride = int(spriteData.stride);
    int format = AGGUtils::toAGGFormat(spriteData.format);
    unsigned char* pixels = static_cast<unsigned char*>(spriteData.pixelData);

    cairo_surface_t* cairoSprite = cairo_image_surface_create_for_data(
      pixels, cairo_format_t(format), spriteData.size.w, spriteData.size.h, stride);

    _cairoSprites[i] = cairoSprite;
  }
  */

  // Initialize AGG.
  {
    BLImageData surfaceData;
    _surface.create(w, h, _params.format);
    _surface.makeMutable(&surfaceData);

    _aggSurface.attach((unsigned char*)surfaceData.pixelData, unsigned(w), unsigned(h), int(surfaceData.stride));
    _pixfmt.attach(_aggSurface);
    _rendererBase.attach(_pixfmt);
    _rendererSolid.attach(_rendererBase);

    _aggSurface.clear(uint32_t(0));
    _rasterizer.clip_box(0, 0, w - 1, h - 1);
  }
}

void AGGModule::onAfterRun() {}

void AGGModule::onDoRectAligned(bool stroke) {
  BLSizeI bounds(_params.screenW, _params.screenH);
  uint32_t style = _params.style;
  int wh = _params.shapeSize;

  _rasterizer.filling_rule(agg::fill_non_zero);

  for (uint32_t i = 0, quantity = _params.quantity; i < quantity; i++) {
    BLRectI rect(_rndCoord.nextRectI(bounds, wh, wh));

    if (!stroke) {
      fillRectAA(rect.x, rect.y, rect.w, rect.h, style);
    }
    else {
      BLRect rectD(rect);
      AGGRectSource r(rectD);
      rasterizePath(r, stroke);
      renderScanlines(rect, style);
    }
  }
}

void AGGModule::onDoRectSmooth(bool stroke) {
  BLSize bounds(_params.screenW, _params.screenH);
  uint32_t style = _params.style;
  double wh = _params.shapeSize;

  _rasterizer.filling_rule(agg::fill_non_zero);

  for (uint32_t i = 0, quantity = _params.quantity; i < quantity; i++) {
    BLRect rect(_rndCoord.nextRect(bounds, wh, wh));
    AGGRectSource r(rect);
    rasterizePath(r, stroke);
    renderScanlines(rect, style);
  }
}

void AGGModule::onDoRectRotated(bool stroke) {
  BLSize bounds(_params.screenW, _params.screenH);
  uint32_t style = _params.style;

  double cx = double(_params.screenW) * 0.5;
  double cy = double(_params.screenH) * 0.5;
  double wh = _params.shapeSize;
  double angle = 0.0;

  _rasterizer.filling_rule(agg::fill_non_zero);

  for (uint32_t i = 0, quantity = _params.quantity; i < quantity; i++, angle += 0.01) {
    BLRect rect(_rndCoord.nextRect(bounds, wh, wh));
    AGGRectSource r(rect);

    agg::trans_affine affine;
    affine.translate(-cx, -cy);
    affine.rotate(angle);
    affine.translate(cx, cy);

    agg::conv_transform<AGGRectSource, agg::trans_affine> transformedRRect(r, affine);

    rasterizePath(transformedRRect, stroke);
    renderScanlines(rect, style);
  }
}

void AGGModule::onDoRoundSmooth(bool stroke) {
  BLSize bounds(_params.screenW, _params.screenH);
  uint32_t style = _params.style;
  double wh = _params.shapeSize;

  _rasterizer.filling_rule(agg::fill_non_zero);

  for (uint32_t i = 0, quantity = _params.quantity; i < quantity; i++) {
    BLRect rect(_rndCoord.nextRect(bounds, wh, wh));
    double radius = _rndExtra.nextDouble(4.0, 40.0);

    agg::rounded_rect r(rect.x, rect.y, rect.x + rect.w, rect.y + rect.h, radius);
    r.normalize_radius();

    rasterizePath(r, stroke);
    renderScanlines(rect, style);
  }
}

void AGGModule::onDoRoundRotated(bool stroke) {
  BLSize bounds(_params.screenW, _params.screenH);
  uint32_t style = _params.style;

  double cx = double(_params.screenW) * 0.5;
  double cy = double(_params.screenH) * 0.5;
  double wh = _params.shapeSize;
  double angle = 0.0;

  _rasterizer.filling_rule(agg::fill_non_zero);

  for (uint32_t i = 0, quantity = _params.quantity; i < quantity; i++, angle += 0.01) {
    BLRect rect(_rndCoord.nextRect(bounds, wh, wh));
    double radius = _rndExtra.nextDouble(4.0, 40.0);

    agg::rounded_rect r(rect.x, rect.y, rect.x + rect.w, rect.y + rect.h, radius);
    r.normalize_radius();

    agg::trans_affine affine;
    affine.translate(-cx, -cy);
    affine.rotate(angle);
    affine.translate(cx, cy);

    agg::conv_transform<agg::rounded_rect, agg::trans_affine> transformedRRect(r, affine);

    rasterizePath(transformedRRect, stroke);
    renderScanlines(rect, style);
  }
}

void AGGModule::onDoPolygon(uint32_t mode, uint32_t complexity) {
  BLSizeI bounds(_params.screenW - _params.shapeSize,
                 _params.screenH - _params.shapeSize);

  uint32_t style = _params.style;
  bool stroke = (mode == 2);
  double wh = _params.shapeSize;

  _rasterizer.filling_rule(mode == 1 ? agg::fill_even_odd : agg::fill_non_zero);

  for (uint32_t i = 0, quantity = _params.quantity; i < quantity; i++) {
    BLPoint base(_rndCoord.nextPoint(bounds));
    AGGRandomPolySource path(base, wh, _rndCoord, complexity);

    rasterizePath(path, stroke);
    renderScanlines(BLRect(base.x, base.y, wh, wh), style);
  }
}

void AGGModule::onDoShape(bool stroke, const BLPoint* pts, size_t count) {
  BLSizeI bounds(_params.screenW - _params.shapeSize,
                 _params.screenH - _params.shapeSize);

  uint32_t style = _params.style;
  double wh = double(_params.shapeSize);

  _rasterizer.filling_rule(agg::fill_non_zero);

  for (uint32_t i = 0, quantity = _params.quantity; i < quantity; i++) {
    BLPoint tr(_rndCoord.nextPoint(bounds));
    AGGShapeDataSource path(pts, count, tr, wh);
    rasterizePath(path, stroke);
    renderScanlines(BLRect(tr.x, tr.y, wh, wh), style);
  }
}

} // {blbench}

#endif // BLEND2D_APPS_ENABLE_AGG

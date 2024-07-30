// This file is part of Blend2D project <https://blend2d.com>
//
// See LICENSE.md for license and copyright information
// SPDX-License-Identifier: Zlib

#ifdef BLEND2D_APPS_ENABLE_QT

#include "app.h"
#include "module_qt.h"

#include <QtCore>
#include <QtGui>

namespace blbench {

static inline QColor toQColor(const BLRgba32& rgba) {
  return QColor(rgba.r(), rgba.g(), rgba.b(), rgba.a());
}

static uint32_t toQtFormat(BLFormat format) {
  switch (format) {
    case BL_FORMAT_PRGB32: return QImage::Format_ARGB32_Premultiplied;
    case BL_FORMAT_XRGB32: return QImage::Format_RGB32;

    default:
      return 0xFFFFFFFFu;
  }
}

static uint32_t toQtOperator(BLCompOp compOp) {
  switch (compOp) {
    case BL_COMP_OP_SRC_OVER   : return QPainter::CompositionMode_SourceOver;
    case BL_COMP_OP_SRC_COPY   : return QPainter::CompositionMode_Source;
    case BL_COMP_OP_SRC_IN     : return QPainter::CompositionMode_SourceIn;
    case BL_COMP_OP_SRC_OUT    : return QPainter::CompositionMode_SourceOut;
    case BL_COMP_OP_SRC_ATOP   : return QPainter::CompositionMode_SourceAtop;
    case BL_COMP_OP_DST_OVER   : return QPainter::CompositionMode_DestinationOver;
    case BL_COMP_OP_DST_COPY   : return QPainter::CompositionMode_Destination;
    case BL_COMP_OP_DST_IN     : return QPainter::CompositionMode_DestinationIn;
    case BL_COMP_OP_DST_OUT    : return QPainter::CompositionMode_DestinationOut;
    case BL_COMP_OP_DST_ATOP   : return QPainter::CompositionMode_DestinationAtop;
    case BL_COMP_OP_XOR        : return QPainter::CompositionMode_Xor;
    case BL_COMP_OP_CLEAR      : return QPainter::CompositionMode_Clear;
    case BL_COMP_OP_PLUS       : return QPainter::CompositionMode_Plus;
    case BL_COMP_OP_MULTIPLY   : return QPainter::CompositionMode_Multiply;
    case BL_COMP_OP_SCREEN     : return QPainter::CompositionMode_Screen;
    case BL_COMP_OP_OVERLAY    : return QPainter::CompositionMode_Overlay;
    case BL_COMP_OP_DARKEN     : return QPainter::CompositionMode_Darken;
    case BL_COMP_OP_LIGHTEN    : return QPainter::CompositionMode_Lighten;
    case BL_COMP_OP_COLOR_DODGE: return QPainter::CompositionMode_ColorDodge;
    case BL_COMP_OP_COLOR_BURN : return QPainter::CompositionMode_ColorBurn;
    case BL_COMP_OP_HARD_LIGHT : return QPainter::CompositionMode_HardLight;
    case BL_COMP_OP_SOFT_LIGHT : return QPainter::CompositionMode_SoftLight;
    case BL_COMP_OP_DIFFERENCE : return QPainter::CompositionMode_Difference;
    case BL_COMP_OP_EXCLUSION  : return QPainter::CompositionMode_Exclusion;

    default:
      return 0xFFFFFFFFu;
  }
}

struct QtModule : public BenchModule {
  QImage* _qtSurface {};
  QImage* _qtSprites[kBenchNumSprites] {};
  QPainter* _qtContext {};

  // Initialized by beforeRun().
  uint32_t _gradientSpread {};

  QtModule();
  ~QtModule() override;

  void serializeInfo(JSONBuilder& json) const override;

  template<typename RectT>
  inline QBrush createBrush(StyleKind style, const RectT& rect);

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

QtModule::QtModule() {
  strcpy(_name, "Qt6");
}
QtModule::~QtModule() {}

void QtModule::serializeInfo(JSONBuilder& json) const {
  json.beforeRecord()
      .addKey("version")
      .addString(qVersion());
}

template<typename RectT>
inline QBrush QtModule::createBrush(StyleKind style, const RectT& rect) {
  switch (style) {
    case StyleKind::kLinearPad:
    case StyleKind::kLinearRepeat:
    case StyleKind::kLinearReflect: {
      double x0 = rect.x + rect.w * 0.2;
      double y0 = rect.y + rect.h * 0.2;
      double x1 = rect.x + rect.w * 0.8;
      double y1 = rect.y + rect.h * 0.8;

      QLinearGradient g((qreal)x0, (qreal)y0, (qreal)x1, (qreal)y1);
      g.setColorAt(qreal(0.0), toQColor(_rndColor.nextRgba32()));
      g.setColorAt(qreal(0.5), toQColor(_rndColor.nextRgba32()));
      g.setColorAt(qreal(1.0), toQColor(_rndColor.nextRgba32()));
      g.setSpread(static_cast<QGradient::Spread>(_gradientSpread));
      return QBrush(g);
    }

    case StyleKind::kRadialPad:
    case StyleKind::kRadialRepeat:
    case StyleKind::kRadialReflect: {
      double cx = rect.x + rect.w / 2;
      double cy = rect.y + rect.h / 2;
      double cr = (rect.w + rect.h) / 4;
      double fx = cx - cr / 2;
      double fy = cy - cr / 2;

      QRadialGradient g(qreal(cx), qreal(cy), qreal(cr), qreal(fx), qreal(fy), qreal(0));
      g.setColorAt(qreal(0.0), toQColor(_rndColor.nextRgba32()));
      g.setColorAt(qreal(0.5), toQColor(_rndColor.nextRgba32()));
      g.setColorAt(qreal(1.0), toQColor(_rndColor.nextRgba32()));
      g.setSpread(static_cast<QGradient::Spread>(_gradientSpread));
      return QBrush(g);
    }

    case StyleKind::kConic: {
      double cx = rect.x + rect.w / 2;
      double cy = rect.y + rect.h / 2;
      QColor c(toQColor(_rndColor.nextRgba32()));

      QConicalGradient g(qreal(cx), qreal(cy), qreal(0));
      g.setColorAt(qreal(0.00), c);
      g.setColorAt(qreal(0.33), toQColor(_rndColor.nextRgba32()));
      g.setColorAt(qreal(0.66), toQColor(_rndColor.nextRgba32()));
      g.setColorAt(qreal(1.00), c);
      return QBrush(g);
    }

    case StyleKind::kPatternNN:
    case StyleKind::kPatternBI:
    default: {
      QBrush brush(*_qtSprites[nextSpriteId()]);

      // FIXME: It seems that Qt will never use subpixel filtering when drawing
      // an unscaled image. The test suite, however, expects that path to be
      // triggered. To fix this, we scale the image slightly (it should have no
      // visual impact) to prevent Qt using nearest-neighbor fast-path.
      qreal scale =  style == StyleKind::kPatternNN ? 1.0 : 1.00001;

      brush.setTransform(QTransform(scale, qreal(0), qreal(0), scale, qreal(rect.x), qreal(rect.y)));
      return brush;
    }
  }
}

bool QtModule::supportsCompOp(BLCompOp compOp) const {
  return toQtOperator(compOp) != 0xFFFFFFFFu;
}

bool QtModule::supportsStyle(StyleKind style) const {
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

void QtModule::beforeRun() {
  int w = int(_params.screenW);
  int h = int(_params.screenH);
  StyleKind style = _params.style;

  // Initialize the sprites.
  for (uint32_t i = 0; i < kBenchNumSprites; i++) {
    const BLImage& sprite = _sprites[i];

    BLImageData spriteData;
    sprite.getData(&spriteData);

    QImage* qtSprite = new QImage(
      static_cast<unsigned char*>(spriteData.pixelData),
      spriteData.size.w,
      spriteData.size.h,
      int(spriteData.stride), QImage::Format_ARGB32_Premultiplied);

    _qtSprites[i] = qtSprite;
  }

  // Initialize the surface and the context.
  BLImageData surfaceData;
  _surface.create(w, h, _params.format);
  _surface.makeMutable(&surfaceData);

  int stride = int(surfaceData.stride);
  int qtFormat = toQtFormat(BLFormat(surfaceData.format));

  _qtSurface = new QImage(
    (unsigned char*)surfaceData.pixelData, w, h,
    stride, static_cast<QImage::Format>(qtFormat));
  if (_qtSurface == nullptr)
    return;

  _qtContext = new QPainter(_qtSurface);
  if (_qtContext == nullptr)
    return;

  // Setup the context.
  _qtContext->setCompositionMode(QPainter::CompositionMode_Source);
  _qtContext->fillRect(0, 0, w, h, QColor(0, 0, 0, 0));

  _qtContext->setCompositionMode(
    static_cast<QPainter::CompositionMode>(
      toQtOperator(_params.compOp)));

  _qtContext->setRenderHint(QPainter::Antialiasing, true);
  _qtContext->setRenderHint(QPainter::SmoothPixmapTransform, _params.style != StyleKind::kPatternNN);

  // Setup globals.
  _gradientSpread = QGradient::PadSpread;

  switch (style) {
    case StyleKind::kLinearPad      : _gradientSpread = QGradient::PadSpread    ; break;
    case StyleKind::kLinearRepeat   : _gradientSpread = QGradient::RepeatSpread ; break;
    case StyleKind::kLinearReflect  : _gradientSpread = QGradient::ReflectSpread; break;
    case StyleKind::kRadialPad      : _gradientSpread = QGradient::PadSpread    ; break;
    case StyleKind::kRadialRepeat   : _gradientSpread = QGradient::RepeatSpread ; break;
    case StyleKind::kRadialReflect  : _gradientSpread = QGradient::ReflectSpread; break;

    default:
      break;
  }
}

void QtModule::flush() {
  // Nothing...
}

void QtModule::afterRun() {
  // Free the surface & the context.
  delete _qtContext;
  delete _qtSurface;

  _qtContext = nullptr;
  _qtSurface = nullptr;

  // Free the sprites.
  for (uint32_t i = 0; i < kBenchNumSprites; i++) {
    delete _qtSprites[i];
    _qtSprites[i] = nullptr;
  }
}

void QtModule::renderRectA(RenderOp op) {
  BLSizeI bounds(_params.screenW, _params.screenH);
  StyleKind style = _params.style;
  int wh = _params.shapeSize;

  if (op == RenderOp::kStroke)
    _qtContext->setBrush(Qt::NoBrush);

  if (style == StyleKind::kSolid) {
    for (uint32_t i = 0, quantity = _params.quantity; i < quantity; i++) {
      BLRectI rect(_rndCoord.nextRectI(bounds, wh, wh));
      QColor color(toQColor(_rndColor.nextRgba32()));

      if (op == RenderOp::kStroke) {
        _qtContext->setPen(color);
        _qtContext->drawRect(QRectF(rect.x, rect.y, rect.w, rect.h));
      }
      else {
        _qtContext->fillRect(QRect(rect.x, rect.y, rect.w, rect.h), color);
      }
    }
  }
  else {
    if ((style == StyleKind::kPatternNN || style == StyleKind::kPatternBI) && op != RenderOp::kStroke) {
      for (uint32_t i = 0, quantity = _params.quantity; i < quantity; i++) {
        BLRectI rect(_rndCoord.nextRectI(bounds, wh, wh));
        const QImage& sprite = *_qtSprites[nextSpriteId()];

        _qtContext->drawImage(QPoint(rect.x, rect.y), sprite);
      }
    }
    else {
      for (uint32_t i = 0, quantity = _params.quantity; i < quantity; i++) {
        BLRectI rect(_rndCoord.nextRectI(bounds, wh, wh));
        QBrush brush(createBrush<BLRectI>(style, rect));

        if (op == RenderOp::kStroke) {
          QPen pen(brush, qreal(_params.strokeWidth));
          pen.setJoinStyle(Qt::MiterJoin);
          _qtContext->setPen(pen);
          _qtContext->drawRect(QRectF(rect.x, rect.y, rect.w, rect.h));
        }
        else {
          _qtContext->fillRect(QRect(rect.x, rect.y, rect.w, rect.h), brush);
        }
      }
    }
  }
}

void QtModule::renderRectF(RenderOp op) {
  BLSize bounds(_params.screenW, _params.screenH);
  StyleKind style = _params.style;
  double wh = _params.shapeSize;

  if (op == RenderOp::kStroke)
    _qtContext->setBrush(Qt::NoBrush);

  if (style == StyleKind::kSolid) {
    for (uint32_t i = 0, quantity = _params.quantity; i < quantity; i++) {
      BLRect rect(_rndCoord.nextRect(bounds, wh, wh));
      QColor color(toQColor(_rndColor.nextRgba32()));

      if (op == RenderOp::kStroke) {
        _qtContext->setPen(color);
        _qtContext->drawRect(QRectF(rect.x, rect.y, rect.w, rect.h));
      }
      else {
        _qtContext->fillRect(QRectF(rect.x, rect.y, rect.w, rect.h), color);
      }
    }
  }
  else {
    for (uint32_t i = 0, quantity = _params.quantity; i < quantity; i++) {
      BLRect rect(_rndCoord.nextRect(bounds, wh, wh));
      QBrush brush(createBrush<BLRect>(style, rect));

      if (op == RenderOp::kStroke) {
        QPen pen(brush, qreal(_params.strokeWidth));
        pen.setJoinStyle(Qt::MiterJoin);

        _qtContext->setPen(pen);
        _qtContext->drawRect(QRectF(rect.x, rect.y, rect.w, rect.h));
      }
      else {
        _qtContext->fillRect(QRectF(rect.x, rect.y, rect.w, rect.h), brush);
      }
    }
  }
}

void QtModule::renderRectRotated(RenderOp op) {
  BLSize bounds(_params.screenW, _params.screenH);
  StyleKind style = _params.style;

  double cx = double(_params.screenW) * 0.5;
  double cy = double(_params.screenH) * 0.5;
  double wh = _params.shapeSize;
  double angle = 0.0;

  if (op == RenderOp::kStroke)
    _qtContext->setBrush(Qt::NoBrush);

  for (uint32_t i = 0, quantity = _params.quantity; i < quantity; i++, angle += 0.01) {
    BLRect rect(_rndCoord.nextRect(bounds, wh, wh));

    QTransform transform;
    transform.translate(cx, cy);
    transform.rotateRadians(angle);
    transform.translate(-cx, -cy);
    _qtContext->setTransform(transform, false);

    if (style == StyleKind::kSolid) {
      QColor color(toQColor(_rndColor.nextRgba32()));

      if (op == RenderOp::kStroke) {
        QPen pen(color, qreal(_params.strokeWidth));
        pen.setJoinStyle(Qt::MiterJoin);

        _qtContext->setPen(pen);
        _qtContext->drawRect(QRectF(rect.x, rect.y, rect.w, rect.h));
      }
      else {
        _qtContext->fillRect(QRectF(rect.x, rect.y, rect.w, rect.h), color);
      }
    }
    else {
      QBrush brush(createBrush<BLRect>(style, rect));

      if (op == RenderOp::kStroke) {
        QPen pen(brush, qreal(_params.strokeWidth));
        pen.setJoinStyle(Qt::MiterJoin);

        _qtContext->setPen(pen);
        _qtContext->drawRect(QRectF(rect.x, rect.y, rect.w, rect.h));
      }
      else {
        _qtContext->fillRect(QRectF(rect.x, rect.y, rect.w, rect.h), brush);
      }
    }

    _qtContext->resetTransform();
  }
}

void QtModule::renderRoundF(RenderOp op) {
  BLSize bounds(_params.screenW, _params.screenH);
  StyleKind style = _params.style;
  double wh = _params.shapeSize;

  if (op == RenderOp::kStroke)
    _qtContext->setBrush(Qt::NoBrush);
  else
    _qtContext->setPen(QPen(Qt::NoPen));

  for (uint32_t i = 0, quantity = _params.quantity; i < quantity; i++) {
    BLRect rect(_rndCoord.nextRect(bounds, wh, wh));
    double radius = _rndExtra.nextDouble(4.0, 40.0);

    if (style == StyleKind::kSolid) {
      QColor color(toQColor(_rndColor.nextRgba32()));

      if (op == RenderOp::kStroke)
        _qtContext->setPen(QPen(color, qreal(_params.strokeWidth)));
      else
        _qtContext->setBrush(QBrush(color));
    }
    else {
      QBrush brush(createBrush<BLRect>(style, rect));

      if (op == RenderOp::kStroke)
        _qtContext->setPen(QPen(brush, qreal(_params.strokeWidth)));
      else
        _qtContext->setBrush(brush);
    }

    _qtContext->drawRoundedRect(
      QRectF(rect.x, rect.y, rect.w, rect.h),
      std::min(rect.w * 0.5, radius),
      std::min(rect.h * 0.5, radius));
  }
}

void QtModule::renderRoundRotated(RenderOp op) {
  BLSize bounds(_params.screenW, _params.screenH);
  StyleKind style = _params.style;

  double cx = double(_params.screenW) * 0.5;
  double cy = double(_params.screenH) * 0.5;
  double wh = _params.shapeSize;
  double angle = 0.0;

  if (op == RenderOp::kStroke)
    _qtContext->setBrush(Qt::NoBrush);
  else
    _qtContext->setPen(QPen(Qt::NoPen));

  for (uint32_t i = 0, quantity = _params.quantity; i < quantity; i++, angle += 0.01) {
    BLRect rect(_rndCoord.nextRect(bounds, wh, wh));
    double radius = _rndExtra.nextDouble(4.0, 40.0);

    QTransform transform;
    transform.translate(cx, cy);
    transform.rotateRadians(angle);
    transform.translate(-cx, -cy);
    _qtContext->setTransform(transform, false);

    if (style == StyleKind::kSolid) {
      QColor color(toQColor(_rndColor.nextRgba32()));
      if (op == RenderOp::kStroke)
        _qtContext->setPen(QPen(color, qreal(_params.strokeWidth)));
      else
        _qtContext->setBrush(QBrush(color));
    }
    else {
      QBrush brush(createBrush<BLRect>(style, rect));
      if (op == RenderOp::kStroke)
        _qtContext->setPen(QPen(brush, qreal(_params.strokeWidth)));
      else
        _qtContext->setBrush(brush);
    }

    _qtContext->drawRoundedRect(
      QRectF(rect.x, rect.y, rect.w, rect.h),
      std::min(rect.w * 0.5, radius),
      std::min(rect.h * 0.5, radius));

    _qtContext->resetTransform();
  }
}

void QtModule::renderPolygon(RenderOp op, uint32_t complexity) {
  BLSizeI bounds(_params.screenW - _params.shapeSize,
                 _params.screenH - _params.shapeSize);
  StyleKind style = _params.style;
  double wh = double(_params.shapeSize);

  Qt::FillRule fillRule = op == RenderOp::kFillEvenOdd ? Qt::OddEvenFill : Qt::WindingFill;

  for (uint32_t i = 0, quantity = _params.quantity; i < quantity; i++) {
    BLPoint base(_rndCoord.nextPoint(bounds));

    double x = _rndCoord.nextDouble(base.x, base.x + wh);
    double y = _rndCoord.nextDouble(base.y, base.y + wh);

    QPainterPath path;
    path.setFillRule(fillRule);
    path.moveTo(x, y);

    for (uint32_t p = 1; p < complexity; p++) {
      x = _rndCoord.nextDouble(base.x, base.x + wh);
      y = _rndCoord.nextDouble(base.y, base.y + wh);
      path.lineTo(x, y);
    }

    if (style == StyleKind::kSolid) {
      QColor color(toQColor(_rndColor.nextRgba32()));

      if (op == RenderOp::kStroke) {
        QPen pen(color, qreal(_params.strokeWidth));
        pen.setJoinStyle(Qt::MiterJoin);
        _qtContext->strokePath(path, pen);
      }
      else {
        _qtContext->fillPath(path, QBrush(color));
      }
    }
    else {
      BLRect rect(base.x, base.y, wh, wh);
      QBrush brush(createBrush<BLRect>(style, rect));

      if (op == RenderOp::kStroke) {
        QPen pen(brush, qreal(_params.strokeWidth));
        pen.setJoinStyle(Qt::MiterJoin);
        _qtContext->strokePath(path, pen);
      }
      else {
        _qtContext->fillPath(path, brush);
      }
    }
  }
}

void QtModule::renderShape(RenderOp op, ShapeData shape) {
  BLSizeI bounds(_params.screenW - _params.shapeSize,
                 _params.screenH - _params.shapeSize);
  StyleKind style = _params.style;
  double wh = double(_params.shapeSize);

  ShapeIterator it(shape);
  QPainterPath path;
  while (it.hasCommand()) {
    if (it.isMoveTo()) {
      path.moveTo(it.x(0) * wh, it.y(0) * wh);
    }
    else if (it.isLineTo()) {
      path.lineTo(it.x(0) * wh, it.y(0) * wh);
    }
    else if (it.isQuadTo()) {
      path.quadTo(
        it.x(0) * wh, it.y(0) * wh,
        it.x(1) * wh, it.y(1) * wh);
    }
    else if (it.isCubicTo()) {
      path.cubicTo(
        it.x(0) * wh, it.y(0) * wh,
        it.x(1) * wh, it.y(1) * wh,
        it.x(2) * wh, it.y(2) * wh);
    }
    else {
      path.closeSubpath();
    }
    it.next();
  }

  Qt::FillRule fillRule = op == RenderOp::kFillEvenOdd ? Qt::OddEvenFill : Qt::WindingFill;

  for (uint32_t i = 0, quantity = _params.quantity; i < quantity; i++) {
    BLPoint base(_rndCoord.nextPoint(bounds));

    _qtContext->save();
    _qtContext->translate(qreal(base.x), qreal(base.y));

    if (style == StyleKind::kSolid) {
      QColor color(toQColor(_rndColor.nextRgba32()));

      if (op == RenderOp::kStroke) {
        QPen pen(color, qreal(_params.strokeWidth));
        pen.setJoinStyle(Qt::MiterJoin);
        _qtContext->strokePath(path, pen);
      }
      else {
        _qtContext->fillPath(path, QBrush(color));
      }
    }
    else {
      BLRect rect(0, 0, wh, wh);
      QBrush brush(createBrush<BLRect>(style, rect));

      if (op == RenderOp::kStroke) {
        QPen pen(brush, qreal(_params.strokeWidth));
        pen.setJoinStyle(Qt::MiterJoin);
        _qtContext->strokePath(path, pen);
      }
      else {
        _qtContext->fillPath(path, brush);
      }
    }

    _qtContext->restore();
  }
}

BenchModule* createQtModule() {
  return new QtModule();
}

} // {blbench}

#endif // BLEND2D_APPS_ENABLE_QT

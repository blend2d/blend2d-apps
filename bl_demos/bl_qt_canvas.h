#ifndef QT_BL_CANVAS_H_INCLUDED
#define QT_BL_CANVAS_H_INCLUDED

#include <blend2d.h>
#include <cmath>
#include <string.h>
#include <stddef.h>
#include <stdint.h>

#include "bl_qt_headers.h"
#include <functional>

class QBLCanvas : public QWidget {
  Q_OBJECT

public:
  QImage qtImage;
  QImage qtImageNonScaling;
  BLImage blImage;

  enum RendererType : uint32_t {
    RendererBlend2D = 0,
    RendererBlend2D_1t = 1,
    RendererBlend2D_2t = 2,
    RendererBlend2D_4t = 4,
    RendererBlend2D_8t = 8,
    RendererBlend2D_12t = 12,
    RendererBlend2D_16t = 16,

    RendererQt = 0xFF
  };

  uint32_t _rendererType {};
  bool _dirty {};
  double _fps {};
  uint32_t _frameCount {};
  QElapsedTimer _elapsedTimer;

  size_t _renderedFrames {};
  size_t _renderTimePos = 31;
  double _renderTime[32] {};

  std::function<void(BLContext& ctx)> onRenderB2D;
  std::function<void(QPainter& ctx)> onRenderQt;
  std::function<void(QMouseEvent*)> onMouseEvent;

  QBLCanvas();
  ~QBLCanvas();

  void resizeEvent(QResizeEvent* event) override;
  void paintEvent(QPaintEvent *event) override;

  void mousePressEvent(QMouseEvent* event) override;
  void mouseReleaseEvent(QMouseEvent* event) override;
  void mouseMoveEvent(QMouseEvent* event) override;

  void setRendererType(uint32_t rendererType);
  void updateCanvas(bool force = false);
  void _resizeCanvas();
  void _renderCanvas();
  void _afterRender();

  inline BLSizeI imageSize() const { return blImage.size(); }
  inline int imageWidth() const { return blImage.width(); }
  inline int imageHeight() const { return blImage.height(); }

  inline uint32_t rendererType() const { return _rendererType; }
  inline double fps() const { return _fps; }

  double lastRenderTime() const;
  double averageRenderTime() const;

  static void initRendererSelectBox(QComboBox* dst, bool blend2DOnly = false);
  static QString rendererTypeToString(uint32_t rendererType);
};

static BL_INLINE QColor blRgbaToQColor(const BLRgba32& rgba) noexcept {
  return QColor(rgba.r(), rgba.g(), rgba.b(), rgba.a());
}

static BL_INLINE QColor blRgbaToQColor(const BLRgba64& rgba) noexcept {
  return QColor(QRgba64::fromRgba64(uint16_t(rgba.r()), uint16_t(rgba.g()), uint16_t(rgba.b()), uint16_t(rgba.a())));
}

static BL_INLINE QPainter::CompositionMode blCompOpToQPainterCompositionMode(BLCompOp compOp) {
  switch (compOp) {
    default:
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
  }
}

static inline BLRgba32 blBackgroundForCompOp(BLCompOp compOp) noexcept {
  switch (compOp) {
    case BL_COMP_OP_SRC_OVER   : return BLRgba32(0xFF000000);
    case BL_COMP_OP_SRC_COPY   : return BLRgba32(0xFF000000);
    case BL_COMP_OP_SRC_IN     : return BLRgba32(0xFFFFFFFF);
    case BL_COMP_OP_SRC_OUT    : return BLRgba32(0x00000000);
    case BL_COMP_OP_SRC_ATOP   : return BLRgba32(0xFFFFFFFF);
    case BL_COMP_OP_DST_OVER   : return BLRgba32(0xFFFFFFFF);
    case BL_COMP_OP_DST_COPY   : return BLRgba32(0xFF000000);
    case BL_COMP_OP_DST_IN     : return BLRgba32(0xFF000000);
    case BL_COMP_OP_DST_OUT    : return BLRgba32(0xFF000000);
    case BL_COMP_OP_DST_ATOP   : return BLRgba32(0xFF000000);
    case BL_COMP_OP_XOR        : return BLRgba32(0xFF000000);
    case BL_COMP_OP_PLUS       : return BLRgba32(0xFF000000);
    case BL_COMP_OP_MULTIPLY   : return BLRgba32(0xFFFFFFFF);
    case BL_COMP_OP_SCREEN     : return BLRgba32(0xFF000000);
    case BL_COMP_OP_OVERLAY    : return BLRgba32(0x00000000);
    case BL_COMP_OP_DARKEN     : return BLRgba32(0xFFFFFFFF);
    case BL_COMP_OP_LIGHTEN    : return BLRgba32(0xFF000000);
    case BL_COMP_OP_COLOR_DODGE: return BLRgba32(0x00000000);
    case BL_COMP_OP_COLOR_BURN : return BLRgba32(0x00000000);
    case BL_COMP_OP_HARD_LIGHT : return BLRgba32(0xFF000000);
    case BL_COMP_OP_SOFT_LIGHT : return BLRgba32(0x00000000);
    case BL_COMP_OP_DIFFERENCE : return BLRgba32(0xFF000000);
    case BL_COMP_OP_EXCLUSION  : return BLRgba32(0xFFFFFFFF);

    default:
      return BLRgba32(0xFF000000);
  }
}

#endif // QT_BL_CANVAS_H_INCLUDED

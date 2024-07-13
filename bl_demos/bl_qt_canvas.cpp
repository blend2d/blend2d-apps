#include "bl_qt_headers.h"
#include "bl_qt_canvas.h"

#include <chrono>

QBLCanvas::QBLCanvas()
  : _rendererType(RendererBlend2D),
    _dirty(true),
    _fps(0),
    _frameCount(0) {
  _elapsedTimer.start();
  setMouseTracking(true);
  setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
}

QBLCanvas::~QBLCanvas() {}

void QBLCanvas::resizeEvent(QResizeEvent* event) {
  _resizeCanvas();
}

void QBLCanvas::paintEvent(QPaintEvent *event) {
  QPainter painter(this);
  if (_dirty)
    _renderCanvas();
  painter.drawImage(QPoint(0, 0), qtImage);
}

void QBLCanvas::mousePressEvent(QMouseEvent* event) {
  if (onMouseEvent)
    onMouseEvent(event);
}

void QBLCanvas::mouseReleaseEvent(QMouseEvent* event) {
  if (onMouseEvent)
    onMouseEvent(event);
}

void QBLCanvas::mouseMoveEvent(QMouseEvent* event) {
  if (onMouseEvent)
    onMouseEvent(event);
}

void QBLCanvas::setRendererType(uint32_t rendererType) {
  _rendererType = rendererType;
  updateCanvas();
}

void QBLCanvas::updateCanvas(bool force) {
  if (force)
    _renderCanvas();
  else
    _dirty = true;
  repaint(0, 0, imageSize().w, imageSize().h);
}

void QBLCanvas::_resizeCanvas() {
  int w = width();
  int h = height();

  float s = devicePixelRatio();
  int sw = int(float(w) * s);
  int sh = int(float(h) * s);

  if (qtImage.width() == sw && qtImage.height() == sh)
    return;

  QImage::Format qimage_format = QImage::Format_ARGB32_Premultiplied;

  qtImage = QImage(sw, sh, qimage_format);
  qtImage.setDevicePixelRatio(s);

  unsigned char* qimage_bits = qtImage.bits();
  qsizetype qimage_stride = qtImage.bytesPerLine();

  qtImageNonScaling = QImage(qimage_bits, sw, sh, qimage_stride, qimage_format);
  blImage.createFromData(sw, sh, BL_FORMAT_PRGB32, qimage_bits, intptr_t(qimage_stride));

  updateCanvas(false);
}

void QBLCanvas::_renderCanvas() {
  auto startTime = std::chrono::high_resolution_clock::now();

  if (_rendererType == RendererQt) {
    if (onRenderQt) {
      QPainter ctx(&qtImageNonScaling);
      onRenderQt(ctx);
    }
  }
  else {
    if (onRenderB2D) {
      // In Blend2D case the non-zero _rendererType specifies the number of threads.
      BLContextCreateInfo createInfo {};
      createInfo.threadCount = _rendererType;

      BLContext ctx(blImage, createInfo);
      onRenderB2D(ctx);
    }
  }

  auto endTime = std::chrono::high_resolution_clock::now();
  std::chrono::duration<double> duration = endTime - startTime;

  _renderTimePos = (_renderTimePos + 1) & 31;
  _renderTime[_renderTimePos] = duration.count();
  _renderedFrames++;

  _dirty = false;
  _afterRender();
}

void QBLCanvas::_afterRender() {
  uint64_t t = _elapsedTimer.elapsed();

  _frameCount++;
  if (t >= 1000) {
    _fps = _frameCount / double(t) * 1000.0;
    _frameCount = 0;
    _elapsedTimer.start();
  }
}

double QBLCanvas::lastRenderTime() const {
  return _renderedFrames > 0 ? _renderTime[_renderTimePos] : 0.0;
}

double QBLCanvas::averageRenderTime() const {
  double sum = 0.0;
  size_t count = _renderedFrames < 32 ? _renderedFrames : size_t(32);

  for (size_t i = 0; i < count; i++) {
    sum += _renderTime[i];
  }

  return (sum * 1000.0) / double(count);
}

void QBLCanvas::initRendererSelectBox(QComboBox* dst, bool blend2DOnly) {
  static const uint32_t rendererTypes[] = {
    RendererQt,
    RendererBlend2D,
    RendererBlend2D_1t,
    RendererBlend2D_2t,
    RendererBlend2D_4t,
    RendererBlend2D_8t,
    RendererBlend2D_12t,
    RendererBlend2D_16t
  };

  for (const auto& rendererType : rendererTypes) {
    if (rendererType == RendererQt && blend2DOnly)
      continue;
    QString s = rendererTypeToString(rendererType);
    dst->addItem(s, QVariant(int(rendererType)));
  }

  dst->setCurrentIndex(blend2DOnly ? 0 : 1);
}

QString QBLCanvas::rendererTypeToString(uint32_t rendererType) {
  char buffer[32];
  switch (rendererType) {
    case RendererQt:
      return QLatin1String("Qt");

    default:
      if (rendererType > 32)
        return QString();

      if (rendererType == 0)
        return QLatin1String("Blend2D");

      snprintf(buffer, sizeof(buffer), "Blend2D %uT", rendererType);
      return QLatin1String(buffer);
  }
}

#include "moc_bl_qt_canvas.cpp"

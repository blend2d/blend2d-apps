#include "qblcanvas.h"

QBLCanvas::QBLCanvas()
  : _rendererType(RendererB2D),
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
  repaint(0, 0, width(), height());
}

void QBLCanvas::_resizeCanvas() {
  int w = width();
  int h = height();

  if (qtImage.width() == w && qtImage.height() == h)
    return;

  qtImage = QImage(w, h, QImage::Format_ARGB32_Premultiplied);
  blImage.createFromData(qtImage.width(), qtImage.height(), BL_FORMAT_PRGB32, qtImage.bits(), qtImage.bytesPerLine());

  updateCanvas(false);
}

void QBLCanvas::_renderCanvas() {
  if (_rendererType == RendererB2D) {
    if (onRenderB2D) {
      BLContext ctx(blImage);
      onRenderB2D(ctx);
    }
  }
  else {
    if (onRenderQt) {
      QPainter ctx(&qtImage);
      onRenderQt(ctx);
    }
  }

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

#include "moc_qblcanvas.cpp"

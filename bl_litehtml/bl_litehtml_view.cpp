#include "bl_litehtml_view.h"
#include "abeezee_regular_ttf.h"

#include <litehtml/master_css.h>

BLLiteHtmlView::BLLiteHtmlView() {
  BLFontFace face;

  // TODO: Hardcoded...
  BLFontData fd;
  fd.createFromData(resource_abeezee_regular_ttf, sizeof(resource_abeezee_regular_ttf));
  face.createFromData(fd, 0);
  _fontManager.addFace(face);

  _htmlContainer.setFontManager(_fontManager);
  _htmlContainer.setFallbackFontFace(BLStringView{"ABeeZee", 7});

  setMouseTracking(true);
  setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);

  setContent(BLStringView{});
}

BLLiteHtmlView::~BLLiteHtmlView() {}

void BLLiteHtmlView::setUrl(const char* url) {
  BLArray<uint8_t> content;
  BLFileSystem::readFile(url, content);

  const char* end = strrchr(url, '/');
  _htmlContainer._baseUrl.assign(url, size_t(end - url));
  _htmlDocument = litehtml::document::createFromString((const char*)content.data(), &_htmlContainer, litehtml::master_css);
  _htmlDocument->render(width());
  htmlHeightChanged(_htmlDocument->height());
}

void BLLiteHtmlView::setContent(BLStringView content) {
  _htmlContainer._baseUrl.assign(":content:");
  _htmlDocument = litehtml::document::createFromString(std::string(content.data, content.size), &_htmlContainer, litehtml::master_css);
  _htmlDocument->render(width());
  htmlHeightChanged(_htmlDocument->height());
}

void BLLiteHtmlView::setScrollY(int y) {
  y = blClamp(y, 0, maxScrollY());

  if (_scrollY != y) {
    _scrollY = y;
    updateCanvas();
  }
}

int BLLiteHtmlView::maxScrollY() const {
  return blMax<int>(_htmlDocument->height() - qtImage.height(), 0);
}

void BLLiteHtmlView::resizeEvent(QResizeEvent* event) {
  _resizeCanvas();
}

void BLLiteHtmlView::paintEvent(QPaintEvent *event) {
  QPainter painter(this);
  if (_dirty)
    _renderCanvas();
  painter.drawImage(QPoint(0, 0), qtImage);
}

void BLLiteHtmlView::mousePressEvent(QMouseEvent* event) {
}

void BLLiteHtmlView::mouseReleaseEvent(QMouseEvent* event) {
}

void BLLiteHtmlView::mouseMoveEvent(QMouseEvent* event) {
}

void BLLiteHtmlView::wheelEvent(QWheelEvent* event) {
  if (event->hasPixelDelta()) {
    QPoint delta = event->pixelDelta();
    setScrollY(_scrollY - delta.y());
    event->accept();
  }
}

void BLLiteHtmlView::updateCanvas(bool force) {
  if (force)
    _renderCanvas();
  else
    _dirty = true;
  repaint(0, 0, width(), height());
}

void BLLiteHtmlView::_resizeCanvas() {
  int w = width();
  int h = height();

  if (qtImage.width() == w && qtImage.height() == h)
    return;

  qtImage = QImage(w, h, QImage::Format_ARGB32_Premultiplied);
  blImage.createFromData(qtImage.width(), qtImage.height(), BL_FORMAT_PRGB32, qtImage.bits(), qtImage.bytesPerLine());
  _htmlContainer.setSize(BLSizeI(w, h));
  _htmlDocument->render(w);
  htmlHeightChanged(_htmlDocument->height());

  updateCanvas(false);
}

void BLLiteHtmlView::_renderCanvas() {
  BLContextCreateInfo createInfo {};
  createInfo.threadCount = 0;

  BLContext ctx(blImage, createInfo);
  litehtml::position clip;
  clip.x = 0;
  clip.y = 0;
  clip.width = width();
  clip.height = height();

  ctx.fillAll(BLRgba32(0xFF000000));
  _htmlDocument->draw((uintptr_t)&ctx, 0, -_scrollY, &clip);
  _dirty = false;
}

#include "moc_bl_litehtml_view.cpp"

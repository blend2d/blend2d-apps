#include "bl_litehtml_view.h"
#include "abeezee_regular_ttf.h"

#include <litehtml/master_css.h>

static BLLiteHtmlDocument::Button translateButtonFromQt(Qt::MouseButton button) {
  switch (button) {
    case Qt::LeftButton: return BLLiteHtmlDocument::Button::kLeft;
    case Qt::RightButton: return BLLiteHtmlDocument::Button::kRight;
    case Qt::MiddleButton: return BLLiteHtmlDocument::Button::kMiddle;
    default: return BLLiteHtmlDocument::Button::kUnknown;
  }
}

static QCursor translateCursorToQt(const char* cursor) {
  if (strcmp(cursor, "auto") == 0)
    return Qt::ArrowCursor;
  else if (strcmp(cursor, "crosshair") == 0)
    return Qt::CrossCursor;
  else if (strcmp(cursor, "pointer") == 0)
    return Qt::PointingHandCursor;
  else if (strcmp(cursor, "text") == 0)
    return Qt::IBeamCursor;
  else
    return Qt::ArrowCursor;
}

BLLiteHtmlView::BLLiteHtmlView(QWidget* parent)
  : QAbstractScrollArea(parent) {

  constexpr int kScrollBarStep = 42;

  BLFontFace face;

  // TODO: Hardcoded...
  BLFontData fd;
  fd.createFromData(resource_abeezee_regular_ttf, sizeof(resource_abeezee_regular_ttf));
  face.createFromData(fd, 0);

  _htmlDoc.fontManager().addFace(face);
  _htmlDoc.setFallbackFamily(face.familyName().view());

  _htmlDoc.onSetCursor = [this](const char* cursor) {
    viewport()->setCursor(translateCursorToQt(cursor));
  };

  _htmlDoc.onLinkClick = [this](const char* resolvedURL) {
    printf("Accepting %s\n", resolvedURL);
    _htmlDoc.acceptLink(resolvedURL);
  };

  horizontalScrollBar()->setSingleStep(kScrollBarStep);
  verticalScrollBar()->setSingleStep(kScrollBarStep);

  setMouseTracking(true);
  setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);

  setContent(BLStringView{});

  connect(horizontalScrollBar(), SIGNAL(valueChanged(int)), SLOT(onScrollBarChanged(int)));
  connect(verticalScrollBar(), SIGNAL(valueChanged(int)), SLOT(onScrollBarChanged(int)));
}

BLLiteHtmlView::~BLLiteHtmlView() {}

void BLLiteHtmlView::_contentAssigned() {
  verticalScrollBar()->setValue(0);
  horizontalScrollBar()->setValue(0);
  _syncViewportInfo();
}

void BLLiteHtmlView::_syncViewportInfo() {
  BLSizeI vs = clientSize();
  _htmlDoc.setViewportSize(vs);

  BLSizeI ds = _htmlDoc.documentSize();

  horizontalScrollBar()->setRange(0, blMax<int>(0, ds.w - vs.w));
  horizontalScrollBar()->setPageStep(vs.w);

  verticalScrollBar()->setRange(0, blMax<int>(0, ds.h - vs.h));
  verticalScrollBar()->setPageStep(vs.h);
}

void BLLiteHtmlView::onScrollBarChanged(int value) {
  (void)value;

  _htmlDoc.setViewportPosition(BLPointI(horizontalScrollBar()->value(), verticalScrollBar()->value()));
  repaintCanvas();
}

void BLLiteHtmlView::setUrl(const char* url) {
  BLStringView masterCSS = BLStringView{litehtml::master_css, strlen(litehtml::master_css)};
  BLStringView url_sv{url, strlen(url)};

  _htmlDoc.createFromURL(url_sv, masterCSS);
  _contentAssigned();
}

void BLLiteHtmlView::setContent(BLStringView content) {
  BLStringView masterCSS = BLStringView{litehtml::master_css, strlen(litehtml::master_css)};
  _htmlDoc.createFromHTML(content, masterCSS);
  _contentAssigned();
}

void BLLiteHtmlView::resizeEvent(QResizeEvent* event) {
  _resizeCanvas();
}

void BLLiteHtmlView::paintEvent(QPaintEvent *event) {
  if (_dirty)
    _renderCanvas();
  QPainter painter(viewport());
  painter.drawImage(QPoint(0, 0), qtImage);
}

static BLPointI mousePositionFromEvent(BLLiteHtmlView* self, QMouseEvent* event) noexcept {
  QPointF pos = event->position();
  BLPointI vp = self->_htmlDoc.viewportPosition();

  return BLPointI(vp.x + int(pos.x()), vp.y + int(pos.y()));
}

void BLLiteHtmlView::mousePressEvent(QMouseEvent* event) {
  BLRectI dirty = _htmlDoc.mouseDown(mousePositionFromEvent(this, event), translateButtonFromQt(event->button()));

  repaintCanvas(dirty);
}

void BLLiteHtmlView::mouseReleaseEvent(QMouseEvent* event) {
  BLRectI dirty = _htmlDoc.mouseUp(mousePositionFromEvent(this, event), translateButtonFromQt(event->button()));

  if (_htmlDoc.linkAccepted()) {
    setUrl(_htmlDoc._acceptedLinkURL.data());
    repaintCanvas();
  }

  repaintCanvas(dirty);
}

void BLLiteHtmlView::mouseMoveEvent(QMouseEvent* event) {
  BLRectI dirty = _htmlDoc.mouseMove(mousePositionFromEvent(this, event));

  repaintCanvas(dirty);
}

void BLLiteHtmlView::repaintCanvas() {
  _dirty = true;
  viewport()->update(0, 0, width(), height());
}

void BLLiteHtmlView::repaintCanvas(const BLRectI& dirty) {
  if (dirty.w > 0 && dirty.h > 0) {
    _dirty = true;
    viewport()->update(dirty.x, dirty.y, dirty.w, dirty.h);
  }
}

void BLLiteHtmlView::_resizeCanvas() {
  BLSizeI size = clientSize();

  if (qtImage.width() != size.w || qtImage.height() != size.h) {
    qtImage = QImage(size.w, size.h, QImage::Format_ARGB32_Premultiplied);
    blImage.createFromData(qtImage.width(), qtImage.height(), BL_FORMAT_PRGB32, qtImage.bits(), qtImage.bytesPerLine());
  }

  _syncViewportInfo();
  repaintCanvas();
}

void BLLiteHtmlView::_renderCanvas() {
  BLContextCreateInfo createInfo {};
  BLContext ctx(blImage, createInfo);

  // ctx.fillAll(BLRgba32(0xFF000000));
  _htmlDoc.draw(ctx);
  _dirty = false;

  updateWindowTitle();
}

void BLLiteHtmlView::updateWindowTitle() {
  if (!_windowToUpdate)
    return;

  BLString title;
  title.appendFormat("%s [Time=%0.2f Avg=%0.2f]", _htmlDoc.url().data(), _htmlDoc.lastFrameDuration(), _htmlDoc.averageFrameDuration());

  _windowToUpdate->setWindowTitle(QString::fromUtf8(title.data(), title.size()));
}

#include "moc_bl_litehtml_view.cpp"

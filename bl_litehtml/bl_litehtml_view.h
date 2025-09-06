#ifndef BL_LITEHTML_VIEW_H_INCLUDED
#define BL_LITEHTML_VIEW_H_INCLUDED

#include <cmath>
#include <string.h>
#include <stddef.h>
#include <stdint.h>

#include <QtGui>
#include <QtWidgets>

#include <blend2d.h>
#include "bl_litehtml_container.h"

class BLLiteHtmlView : public QAbstractScrollArea {
  Q_OBJECT

public:
  QImage qt_image;
  BLImage bl_image;
  BLLiteHtmlDocument _htmlDoc;
  bool _dirty = true;
  QWidget* _windowToUpdate {};

  explicit BLLiteHtmlView(QWidget* parent = nullptr);
  ~BLLiteHtmlView();

private:
  void _contentAssigned();
  void _syncViewportInfo();

public:
  void setUrl(const char* url);
  void setContent(BLStringView content);

  void resizeEvent(QResizeEvent* event) override;
  void paintEvent(QPaintEvent *event) override;

  void mousePressEvent(QMouseEvent* event) override;
  void mouseReleaseEvent(QMouseEvent* event) override;
  void mouseMoveEvent(QMouseEvent* event) override;

  void repaintCanvas();
  void repaintCanvas(const BLRectI& dirty);
  void _resize_canvas();
  void _render_canvas();

  inline void setWindowToUpdate(QWidget* widget) noexcept {
    _windowToUpdate = widget;
  }

  inline BLPointI scrollPosition() const {
    return BLPointI(horizontalScrollBar()->value(), verticalScrollBar()->value());
  }

  inline BLSizeI clientSize() const {
    QSize sz = viewport()->size();
    return BLSizeI(sz.width(), sz.height());
  }

  void updateWindowTitle();

public Q_SLOTS:
  void onScrollBarChanged(int value);
};

#endif // BL_LITEHTML_VIEW_H_INCLUDED

#ifndef BL_LITEHTML_VIEW_H_INCLUDED
#define BL_LITEHTML_VIEW_H_INCLUDED

#include <cmath>
#include <string.h>
#include <stddef.h>
#include <stdint.h>
#include <functional>

#include <QtGui>
#include <QtWidgets>

#include <blend2d.h>
#include "bl_litehtml_container.h"

class BLLiteHtmlView : public QWidget {
  Q_OBJECT

public:
  QImage qtImage;
  BLImage blImage;
  BLFontManager _fontManager;
  litehtml::document::ptr _htmlDocument;
  BLLiteHtmlContainer _htmlContainer;
  bool _dirty = true;

  int _scrollY = 0;

  BLLiteHtmlView();
  ~BLLiteHtmlView();

  void setUrl(const char* url);
  void setContent(BLStringView content);

  void setScrollY(int y);
  int maxScrollY() const;

  void resizeEvent(QResizeEvent* event) override;
  void paintEvent(QPaintEvent *event) override;

  void mousePressEvent(QMouseEvent* event) override;
  void mouseReleaseEvent(QMouseEvent* event) override;
  void mouseMoveEvent(QMouseEvent* event) override;
  void wheelEvent(QWheelEvent* event) override;

  void updateCanvas(bool force = false);
  void _resizeCanvas();
  void _renderCanvas();
  void _afterRender();

Q_SIGNALS:
  void htmlHeightChanged(int newValue);
};

#endif // BL_LITEHTML_VIEW_H_INCLUDED

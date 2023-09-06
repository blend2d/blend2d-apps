#include <stdlib.h>
#include <vector>

#include "bl_qt_headers.h"
#include "bl_qt_canvas.h"

class MainWindow : public QWidget {
  Q_OBJECT

public:
  QTimer _timer;
  QSlider _sizeSlider;
  QSlider _countSlider;
  QComboBox _rendererSelect;
  QComboBox _compOpSelect;
  QComboBox _shapeTypeSelect;
  QCheckBox _limitFpsCheck;
  QBLCanvas _canvas;

  BLRandom _random;
  std::vector<BLPoint> _coords;
  std::vector<BLPoint> _steps;
  std::vector<BLRgba32> _colors;
  BLCompOp _compOp = BL_COMP_OP_SRC_OVER;
  uint32_t _shapeType = 0;
  double _rectSize = 64.0;

  enum ShapeType {
    kShapeRect,
    kShapeRectPath,
    kShapeRoundRect,
    kShapePolyPath,
  };

  MainWindow() : _random(0x1234) {
    QVBoxLayout* vBox = new QVBoxLayout();
    vBox->setContentsMargins(0, 0, 0, 0);
    vBox->setSpacing(0);

    QGridLayout* grid = new QGridLayout();
    grid->setContentsMargins(5, 5, 5, 5);
    grid->setSpacing(5);

    QBLCanvas::initRendererSelectBox(&_rendererSelect);
    _compOpSelect.addItem("SrcOver", QVariant(int(BL_COMP_OP_SRC_OVER)));
    _compOpSelect.addItem("SrcCopy", QVariant(int(BL_COMP_OP_SRC_COPY)));
    _compOpSelect.addItem("DstAtop", QVariant(int(BL_COMP_OP_DST_ATOP)));
    _compOpSelect.addItem("Xor", QVariant(int(BL_COMP_OP_XOR)));
    _compOpSelect.addItem("Plus", QVariant(int(BL_COMP_OP_PLUS)));
    _compOpSelect.addItem("Screen", QVariant(int(BL_COMP_OP_SCREEN)));
    _compOpSelect.addItem("Lighten", QVariant(int(BL_COMP_OP_LIGHTEN)));
    _compOpSelect.addItem("Hard Light", QVariant(int(BL_COMP_OP_HARD_LIGHT)));
    _compOpSelect.addItem("Difference", QVariant(int(BL_COMP_OP_DIFFERENCE)));

    _shapeTypeSelect.addItem("Rect", QVariant(int(kShapeRect)));
    _shapeTypeSelect.addItem("RectPath", QVariant(int(kShapeRectPath)));
    _shapeTypeSelect.addItem("RoundRect", QVariant(int(kShapeRoundRect)));
    _shapeTypeSelect.addItem("Polygon", QVariant(int(kShapePolyPath)));

    _limitFpsCheck.setText(QLatin1String("Limit FPS"));

    _sizeSlider.setOrientation(Qt::Horizontal);
    _sizeSlider.setMinimum(8);
    _sizeSlider.setMaximum(128);
    _sizeSlider.setSliderPosition(64);

    _countSlider.setOrientation(Qt::Horizontal);
    _countSlider.setMinimum(1);
    _countSlider.setMaximum(10000);
    _countSlider.setSliderPosition(200);

    _canvas.onRenderB2D = std::bind(&MainWindow::onRenderB2D, this, std::placeholders::_1);
    _canvas.onRenderQt = std::bind(&MainWindow::onRenderQt, this, std::placeholders::_1);

    connect(&_rendererSelect, SIGNAL(activated(int)), SLOT(onRendererChanged(int)));
    connect(&_compOpSelect, SIGNAL(activated(int)), SLOT(onCompOpChanged(int)));
    connect(&_shapeTypeSelect, SIGNAL(activated(int)), SLOT(onShapeTypeChanged(int)));
    connect(&_limitFpsCheck, SIGNAL(stateChanged(int)), SLOT(onLimitFpsChanged(int)));
    connect(&_sizeSlider, SIGNAL(valueChanged(int)), SLOT(onSizeChanged(int)));
    connect(&_countSlider, SIGNAL(valueChanged(int)), SLOT(onCountChanged(int)));

    grid->addWidget(new QLabel("Renderer:"), 0, 0);
    grid->addWidget(&_rendererSelect, 0, 1);

    grid->addWidget(new QLabel("Comp Op:"), 0, 2);
    grid->addWidget(&_compOpSelect, 0, 3);

    grid->addWidget(new QLabel("Shape:"), 0, 4);
    grid->addWidget(&_shapeTypeSelect, 0, 5);

    grid->addItem(new QSpacerItem(0, 0, QSizePolicy::Expanding), 0, 6);
    grid->addWidget(&_limitFpsCheck, 0, 7, Qt::AlignRight);

    grid->addWidget(new QLabel("Count:"), 1, 0, 1, 1, Qt::AlignRight);
    grid->addWidget(&_countSlider, 1, 1, 1, 7);

    grid->addWidget(new QLabel("Size:"), 2, 0, 1, 1, Qt::AlignRight);
    grid->addWidget(&_sizeSlider, 2, 1, 1, 7);

    vBox->addLayout(grid);
    vBox->addWidget(&_canvas);
    setLayout(vBox);

    connect(&_timer, SIGNAL(timeout()), this, SLOT(onTimer()));
    onInit();
  }

  void showEvent(QShowEvent* event) override { _timer.start(); }
  void hideEvent(QHideEvent* event) override { _timer.stop(); }
  void keyPressEvent(QKeyEvent* event) override {}

  void onInit() {
    setCount(_countSlider.sliderPosition());
    _limitFpsCheck.setChecked(true);
    _updateTitle();
  }

  double randomSign() noexcept { return _random.nextDouble() < 0.5 ? 1.0 : -1.0; }
  BLRgba32 randomColor() noexcept { return BLRgba32(_random.nextUInt32()); }

  Q_SLOT void onRendererChanged(int index) { _canvas.setRendererType(_rendererSelect.itemData(index).toInt());  }
  Q_SLOT void onCompOpChanged(int index) { _compOp = (BLCompOp)_compOpSelect.itemData(index).toInt(); };
  Q_SLOT void onShapeTypeChanged(int index) { _shapeType = _shapeTypeSelect.itemData(index).toInt(); };
  Q_SLOT void onLimitFpsChanged(int value) { _timer.setInterval(value ? 1000 / 120 : 0); }
  Q_SLOT void onSizeChanged(int value) { _rectSize = value; }
  Q_SLOT void onCountChanged(int value) { setCount(size_t(value)); }

  Q_SLOT void onTimer() {
    double w = _canvas.blImage.width();
    double h = _canvas.blImage.height();

    size_t size = _coords.size();
    for (size_t i = 0; i < size; i++) {
      BLPoint& vertex = _coords[i];
      BLPoint& step = _steps[i];

      vertex += step;
      if (vertex.x <= 0.0 || vertex.x >= w) {
        step.x = -step.x;
        vertex.x = blMin(vertex.x + step.x, w);
      }

      if (vertex.y <= 0.0 || vertex.y >= h) {
        step.y = -step.y;
        vertex.y = blMin(vertex.y + step.y, h);
      }
    }

    _canvas.updateCanvas(true);
    _updateTitle();
  }

  void onRenderB2D(BLContext& ctx) noexcept {
    ctx.fillAll(BLRgba32(0xFF000000));

    size_t i;
    size_t size = _coords.size();

    double rectSize = _rectSize;
    double halfSize = _rectSize * 0.5;

    ctx.setCompOp(_compOp);

    switch (_shapeType) {
      case kShapeRect:
        for (i = 0; i < size; i++) {
          double x = _coords[i].x - halfSize;
          double y = _coords[i].y - halfSize;
          ctx.fillRect(_coords[i].x - halfSize, _coords[i].y - halfSize, rectSize, rectSize, _colors[i]);
        }
        break;

      case kShapeRectPath:
        for (i = 0; i < size; i++) {
          double x = _coords[i].x - halfSize;
          double y = _coords[i].y - halfSize;

          BLPath path;
          path.addRect(x, y, rectSize, rectSize);
          ctx.fillPath(path, _colors[i]);
        }
        break;

      case kShapePolyPath:
        for (i = 0; i < size; i++) {
          double x = _coords[i].x - halfSize;
          double y = _coords[i].y - halfSize;

          BLPath path;
          path.moveTo(x + rectSize / 2, y);
          path.lineTo(x + rectSize, y + rectSize / 3);
          path.lineTo(x + rectSize - rectSize / 3, y + rectSize);
          path.lineTo(x + rectSize / 3, y + rectSize);
          path.lineTo(x, y + rectSize / 3);
          ctx.fillPath(path, _colors[i]);
        }
        break;

      case kShapeRoundRect:
        for (i = 0; i < size; i++) {
          double x = _coords[i].x - halfSize;
          double y = _coords[i].y - halfSize;

          ctx.fillRoundRect(BLRoundRect(x, y, rectSize, rectSize, 10), _colors[i]);
        }
        break;
    }
}

  void onRenderQt(QPainter& ctx) noexcept {
    ctx.fillRect(0, 0, _canvas.width(), _canvas.height(), QColor(0, 0, 0));

    ctx.setRenderHint(QPainter::Antialiasing, true);
    ctx.setCompositionMode(blCompOpToQPainterCompositionMode(_compOp));

    size_t i;
    size_t size = _coords.size();

    double rectSize = _rectSize;
    double halfSize = _rectSize * 0.5;

    switch (_shapeType) {
      case kShapeRect:
        for (i = 0; i < size; i++) {
          double x = _coords[i].x - halfSize;
          double y = _coords[i].y - halfSize;
          ctx.fillRect(QRectF(_coords[i].x - halfSize, _coords[i].y - halfSize, rectSize, rectSize), blRgbaToQColor(_colors[i]));
        }
        break;

      case kShapeRectPath:
        for (i = 0; i < size; i++) {
          double x = _coords[i].x - halfSize;
          double y = _coords[i].y - halfSize;

          QPainterPath path;
          path.addRect(x, y, rectSize, rectSize);
          ctx.fillPath(path, blRgbaToQColor(_colors[i]));
        }
        break;

      case kShapePolyPath:
        for (i = 0; i < size; i++) {
          double x = _coords[i].x - halfSize;
          double y = _coords[i].y - halfSize;

          QPainterPath path;
          path.moveTo(x + rectSize / 2, y);
          path.lineTo(x + rectSize, y + rectSize / 3);
          path.lineTo(x + rectSize - rectSize / 3, y + rectSize);
          path.lineTo(x + rectSize / 3, y + rectSize);
          path.lineTo(x, y + rectSize / 3);
          ctx.fillPath(path, blRgbaToQColor(_colors[i]));
        }
        break;

      case kShapeRoundRect:
        for (i = 0; i < size; i++) {
          double x = _coords[i].x - halfSize;
          double y = _coords[i].y - halfSize;

          QPainterPath path;
          path.addRoundedRect(QRectF(x, y, rectSize, rectSize), 10, 10);
          ctx.fillPath(path, blRgbaToQColor(_colors[i]));
        }
        break;
    }
  }

  void setCount(size_t size) {
    double w = _canvas.blImage.width();
    double h = _canvas.blImage.height();
    size_t i = _coords.size();

    _coords.resize(size);
    _steps.resize(size);
    _colors.resize(size);

    while (i < size) {
      _coords[i].reset(_random.nextDouble() * w,
                       _random.nextDouble() * h);
      _steps[i].reset((_random.nextDouble() * 0.5 + 0.05) * randomSign(),
                      (_random.nextDouble() * 0.5 + 0.05) * randomSign());
      _colors[i].reset(randomColor());
      i++;
    }
  }

  void _updateTitle() {
    char buf[256];
    snprintf(buf, 256, "Rectangles [%dx%d] [Size=%d N=%zu] [AvgTime=%.2fms FPS=%.1f]",
      _canvas.width(),
      _canvas.height(),
      int(_rectSize),
      _coords.size(),
      _canvas.averageRenderTime(),
      _canvas.fps());

    QString title = QString::fromUtf8(buf);
    if (title != windowTitle())
      setWindowTitle(title);
  }
};

int main(int argc, char *argv[]) {
  QApplication app(argc, argv);
  MainWindow win;

  win.setMinimumSize(QSize(400, 320));
  win.resize(QSize(580, 520));
  win.show();

  return app.exec();
}

#include "bl_qt_rects.moc"

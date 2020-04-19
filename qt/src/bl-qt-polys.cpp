#include "qblcanvas.h"

#include <stdlib.h>
#include <vector>

// ============================================================================
// [MainWindow]
// ============================================================================

class MainWindow : public QWidget {
  Q_OBJECT

public:
  QTimer _timer;
  QSlider _slider;
  QBLCanvas _canvas;
  QComboBox _rendererSelect;
  QCheckBox _limitFpsCheck;
  QComboBox _operationSelect;

  int _op;
  std::vector<BLPoint> _poly;
  std::vector<BLPoint> _step;

  BLRandom _random;

  MainWindow() :
    _op(0),
    _random(0x1234) {
    QVBoxLayout* vBox = new QVBoxLayout();
    vBox->setContentsMargins(0, 0, 0, 0);
    vBox->setSpacing(0);

    QGridLayout* grid = new QGridLayout();
    grid->setContentsMargins(5, 5, 5, 5);
    grid->setSpacing(5);

    QBLCanvas::initRendererSelectBox(&_rendererSelect);
    _limitFpsCheck.setText(QLatin1Literal("Limit FPS"));

    _operationSelect.addItem("Fill Poly", QVariant(int(0)));
    _operationSelect.addItem("Stroke Poly [W=1]", QVariant(int(1)));
    _operationSelect.addItem("Stroke Poly [W=3]", QVariant(int(2)));
    _operationSelect.addItem("Stroke Poly [W=5]", QVariant(int(3)));

    _operationSelect.addItem("Fill Cubics", QVariant(int(4)));
    _operationSelect.addItem("Stroke Cubics [W=1]", QVariant(int(4)));
    _operationSelect.addItem("Stroke Cubics [W=3]", QVariant(int(5)));
    _operationSelect.addItem("Stroke Cubics [W=5]", QVariant(int(6)));

    _slider.setOrientation(Qt::Horizontal);
    _slider.setMinimum(3);
    _slider.setMaximum(1000);
    _slider.setSliderPosition(10);

    _canvas.onRenderB2D = std::bind(&MainWindow::onRenderB2D, this, std::placeholders::_1);
    _canvas.onRenderQt = std::bind(&MainWindow::onRenderQt, this, std::placeholders::_1);

    connect(&_rendererSelect, SIGNAL(activated(int)), SLOT(onRendererChanged(int)));
    connect(&_limitFpsCheck, SIGNAL(stateChanged(int)), SLOT(onLimitFpsChanged(int)));
    connect(&_operationSelect, SIGNAL(activated(int)), SLOT(onOperationChanged(int)));
    connect(&_slider, SIGNAL(valueChanged(int)), SLOT(onPolySizeChanged(int)));

    grid->addWidget(new QLabel("Renderer:"), 0, 0);
    grid->addWidget(&_rendererSelect, 0, 1);
    grid->addWidget(new QLabel("Operation:"), 0, 2);
    grid->addWidget(&_operationSelect, 0, 3);

    grid->addItem(new QSpacerItem(0, 0, QSizePolicy::Expanding), 0, 4);
    grid->addWidget(&_limitFpsCheck, 0, 5);

    grid->addWidget(new QLabel("Count:"), 1, 0, Qt::AlignRight);
    grid->addWidget(&_slider, 1, 1, 1, 5);

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
    setPolySize(50);
    _limitFpsCheck.setChecked(true);
    _updateTitle();
  }

  double randomSign() noexcept { return _random.nextDouble() < 0.5 ? 1.0 : -1.0; }

  Q_SLOT void onRendererChanged(int index) { _canvas.setRendererType(_rendererSelect.itemData(index).toInt());  }
  Q_SLOT void onLimitFpsChanged(int value) { _timer.setInterval(value ? 1000 / 120 : 0); }
  Q_SLOT void onOperationChanged(int index) { _op = index; }
  Q_SLOT void onPolySizeChanged(int value) { setPolySize(size_t(value)); }

  Q_SLOT void onTimer() {
    double w = _canvas.blImage.width();
    double h = _canvas.blImage.height();

    size_t size = _poly.size();
    for (size_t i = 0; i < size; i++) {
      BLPoint& vertex = _poly[i];
      BLPoint& step = _step[i];

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
    ctx.setFillStyle(BLRgba32(0xFF000000));
    ctx.fillAll();

    ctx.setFillRule(BL_FILL_RULE_EVEN_ODD);
    ctx.setFillStyle(BLRgba32(0xFFFFFFFF));
    ctx.setStrokeStyle(BLRgba32(0xFFFFFFFF));

    switch (_op) {
      case 0: {
        ctx.fillPolygon(_poly.data(), _poly.size());
        break;
      }

      case 1:
      case 2:
      case 3: {
        ctx.setStrokeWidth((_op - 1) * 2 + 1);
        ctx.strokePolygon(_poly.data(), _poly.size());
        break;
      }

      case 4:
      case 5:
      case 6:
      case 7: {
        const BLPoint* poly = _poly.data();
        size_t n = _poly.size();
        BLPath path;
        path.moveTo(poly[0]);
        for (size_t i = 4; i < n; i += 3)
          path.cubicTo(poly[i - 3], poly[i - 2], poly[i - 1]);

        if (_op == 4) {
          ctx.fillPath(path);
        }
        else {
          ctx.setStrokeWidth((_op - 5) * 2 + 1);
          ctx.strokePath(path);
        }
        break;
      }
    }
  }

  void onRenderQt(QPainter& ctx) noexcept {
    ctx.fillRect(0, 0, _canvas.width(), _canvas.height(), QColor(0, 0, 0));

    ctx.setRenderHint(QPainter::Antialiasing, true);
    ctx.setBrush(QColor(255, 255, 255));

    QPen pen(QColor(255, 255, 255));
    pen.setJoinStyle(Qt::MiterJoin);

    switch (_op) {
      case 0: {
        ctx.setPen(Qt::NoPen);
        ctx.drawPolygon(reinterpret_cast<const QPointF*>(_poly.data()), int(_poly.size()), Qt::OddEvenFill);
        break;
      }

      case 1:
      case 2:
      case 3:
        pen.setWidth((_op - 1) * 2 + 1);
        ctx.setBrush(Qt::NoBrush);
        ctx.setPen(pen);
        ctx.drawPolygon(reinterpret_cast<const QPointF*>(_poly.data()), int(_poly.size()), Qt::OddEvenFill);
        break;

      case 4:
      case 5:
      case 6:
      case 7: {
        const BLPoint* poly = _poly.data();
        size_t n = _poly.size();
        QPainterPath path;
        path.moveTo(poly[0].x, poly[0].y);
        for (size_t i = 4; i < n; i += 3)
          path.cubicTo(poly[i - 3].x, poly[i - 3].y, poly[i - 2].x, poly[i - 2].y, poly[i - 1].x, poly[i - 1].y);

        if (_op == 4) {
          path.setFillRule(Qt::OddEvenFill);
          ctx.fillPath(path, QColor(255, 255, 255));
        }
        else {
          pen.setWidth((_op - 5) * 2 + 1);
          ctx.strokePath(path, pen);
        }
        break;
      }
    }
  }

  void setPolySize(size_t size) {
    double w = _canvas.blImage.width();
    double h = _canvas.blImage.height();
    size_t prev = _poly.size();

    _poly.resize(size);
    _step.resize(size);

    while (prev < size) {
      _poly[prev].reset(_random.nextDouble() * w,
                        _random.nextDouble() * h);
      _step[prev].reset((_random.nextDouble() * 0.5 + 0.05) * randomSign(),
                        (_random.nextDouble() * 0.5 + 0.05) * randomSign());
      prev++;
    }
  }

  void _updateTitle() {
    char buf[256];
    snprintf(buf, 256, "Polygons Sample [%dx%d] [N=%zu] [%.1f FPS]",
      _canvas.width(),
      _canvas.height(),
      _poly.size(),
      _canvas.fps());

    QString title = QString::fromUtf8(buf);
    if (title != windowTitle())
      setWindowTitle(title);
  }
};

// ============================================================================
// [Main]
// ============================================================================

int main(int argc, char *argv[]) {
  QApplication app(argc, argv);
  MainWindow win;

  win.setMinimumSize(QSize(400, 320));
  win.resize(QSize(580, 520));
  win.show();

  return app.exec();
}

#include "bl-qt-polys.moc"

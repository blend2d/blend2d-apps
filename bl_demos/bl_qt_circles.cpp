#include <stdlib.h>
#include <cmath>
#include <vector>

#include "bl_qt_headers.h"
#include "bl_qt_canvas.h"

class MainWindow : public QWidget {
  Q_OBJECT

public:
  QTimer _timer;
  QComboBox _rendererSelect;
  QCheckBox _limitFpsCheck;
  QSlider _countSlider;
  QBLCanvas _canvas;

  bool _animate = true;
  double _angle {};
  int _count {};

  MainWindow() {
    QVBoxLayout* vBox = new QVBoxLayout();
    vBox->setContentsMargins(0, 0, 0, 0);
    vBox->setSpacing(0);

    QGridLayout* grid = new QGridLayout();
    grid->setContentsMargins(5, 5, 5, 5);
    grid->setSpacing(5);

    QBLCanvas::initRendererSelectBox(&_rendererSelect);
    _limitFpsCheck.setText(QLatin1String("Limit FPS"));

    _countSlider.setMinimum(100);
    _countSlider.setMaximum(2000);
    _countSlider.setValue(500);
    _countSlider.setOrientation(Qt::Horizontal);

    connect(&_rendererSelect, SIGNAL(activated(int)), SLOT(onRendererChanged(int)));
    connect(&_limitFpsCheck, SIGNAL(stateChanged(int)), SLOT(onLimitFpsChanged(int)));

    grid->addWidget(new QLabel("Renderer:"), 0, 0);
    grid->addWidget(&_rendererSelect, 0, 1);

    grid->addItem(new QSpacerItem(0, 0, QSizePolicy::Expanding), 0, 2);
    grid->addWidget(&_limitFpsCheck, 0, 3, Qt::AlignRight);

    grid->addWidget(new QLabel("Count:"), 1, 0, Qt::AlignRight);
    grid->addWidget(&_countSlider, 1, 1, 1, 4);

    _canvas.onRenderB2D = std::bind(&MainWindow::onRenderB2D, this, std::placeholders::_1);
    _canvas.onRenderQt = std::bind(&MainWindow::onRenderQt, this, std::placeholders::_1);

    vBox->addItem(grid);
    vBox->addWidget(&_canvas);
    setLayout(vBox);

    connect(&_timer, SIGNAL(timeout()), this, SLOT(onTimer()));
    connect(new QShortcut(QKeySequence(Qt::Key_P), this), SIGNAL(activated()), SLOT(onToggleAnimate()));

    onInit();
  }

  void showEvent(QShowEvent* event) override { _timer.start(); }
  void hideEvent(QHideEvent* event) override { _timer.stop(); }
  void keyPressEvent(QKeyEvent* event) override {}

  void onInit() {
    _angle = 0;
    _count = 0;
    _limitFpsCheck.setChecked(true);
    _updateTitle();
  }

  Q_SLOT void onToggleAnimate() { _animate = !_animate; }
  Q_SLOT void onRendererChanged(int index) { _canvas.setRendererType(_rendererSelect.itemData(index).toInt()); }
  Q_SLOT void onLimitFpsChanged(int value) { _timer.setInterval(value ? 1000 / 120 : 0); }

  Q_SLOT void onTimer() {
    if (_animate) {
      _angle += 0.05;
      if (_angle >= 360)
        _angle -= 360;
    }

    _canvas.updateCanvas(true);
    _updateTitle();
  }

  // The idea is based on:
  //   https://github.com/fogleman/gg/blob/master/examples/spiral.go

  void onRenderB2D(BLContext& ctx) noexcept {
    ctx.fillAll(BLRgba32(0xFF000000u));

    BLPath p;

    int count = _countSlider.value();
    double PI = 3.14159265359;

    double cx = _canvas.width() / 2;
    double cy = _canvas.height() / 2;
    double maxDist = 1000.0;
    double baseAngle = _angle / 180.0 * PI;

    for (int i = 0; i < count; i++) {
      double t = double(i) * 1.01 / 1000;
      double d = t * 1000.0 * 0.4 + 10;
      double a = baseAngle + t * PI * 2 * 20;
      double x = cx + std::cos(a) * d;
      double y = cy + std::sin(a) * d;
      double r = std::min(t * 8 + 0.5, 10.0);
      p.addCircle(BLCircle(x, y, r));
    }

    ctx.fillPath(p, BLRgba32(0xFFFFFFFFu));
  }

  void onRenderQt(QPainter& ctx) noexcept {
    ctx.fillRect(0, 0, _canvas.width(), _canvas.height(), QColor(0, 0, 0));
    ctx.setRenderHint(QPainter::Antialiasing, true);

    QPainterPath p;
    QBrush brush(QColor(qRgb(255, 255, 255)));

    int count = _countSlider.value();
    double PI = 3.14159265359;

    double cx = _canvas.width() / 2;
    double cy = _canvas.height() / 2;
    double baseAngle = _angle / 180.0 * PI;

    for (int i = 0; i < count; i++) {
      double t = double(i) * 1.01 / 1000;
      double d = t * 1000.0 * 0.4 + 10;
      double a = baseAngle + t * PI * 2 * 20;
      double x = cx + std::cos(a) * d;
      double y = cy + std::sin(a) * d;
      double r = std::min(t * 8 + 0.5, 10.0);
      p.addEllipse(x - r, y - r, r * 2.0, r * 2.0);
    }

    ctx.fillPath(p, brush);
  }

  void _updateTitle() {
    char buf[256];
    snprintf(buf, 256, "Circles Sample [%dx%d] [%d circles] [AvgTime=%.2fms FPS=%.1f]",
      _canvas.width(),
      _canvas.height(),
      _countSlider.value(),
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

#include "bl_qt_circles.moc"

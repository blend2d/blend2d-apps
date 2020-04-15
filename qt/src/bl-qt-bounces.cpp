#include "qblcanvas.h"

#include <stdlib.h>

// ============================================================================
// [MainWindow]
// ============================================================================

class MainWindow : public QWidget {
  Q_OBJECT

public:
  QTimer _timer;
  QBLCanvas _canvas;
  QComboBox _rendererSelect;
  QCheckBox _limitFpsCheck;
  double _time;
  int _count;

  MainWindow() {
    QVBoxLayout* vBox = new QVBoxLayout();
    vBox->setContentsMargins(0, 0, 0, 0);
    vBox->setSpacing(0);

    QGridLayout* grid = new QGridLayout();
    grid->setContentsMargins(5, 5, 5, 5);
    grid->setSpacing(5);

    QBLCanvas::initRendererSelectBox(&_rendererSelect);
    _limitFpsCheck.setText(QLatin1Literal("Limit FPS"));

    connect(&_rendererSelect, SIGNAL(activated(int)), SLOT(onRendererChanged(int)));
    connect(&_limitFpsCheck, SIGNAL(stateChanged(int)), SLOT(onLimitFpsChanged(int)));

    grid->addWidget(new QLabel("Renderer:"), 0, 0);
    grid->addWidget(&_rendererSelect, 0, 1);

    grid->addItem(new QSpacerItem(0, 0, QSizePolicy::Expanding), 0, 2);
    grid->addWidget(&_limitFpsCheck, 0, 3, Qt::AlignRight);

    _canvas.onRenderB2D = std::bind(&MainWindow::onRenderB2D, this, std::placeholders::_1);
    _canvas.onRenderQt = std::bind(&MainWindow::onRenderQt, this, std::placeholders::_1);

    vBox->addItem(grid);
    vBox->addWidget(&_canvas);
    setLayout(vBox);

    connect(&_timer, SIGNAL(timeout()), this, SLOT(onTimer()));
    onInit();
  }

  void showEvent(QShowEvent* event) override { _timer.start(); }
  void hideEvent(QHideEvent* event) override { _timer.stop(); }
  void keyPressEvent(QKeyEvent* event) override {}

  void onInit() {
    _time = 0;
    _count = 0;
    _limitFpsCheck.setChecked(true);
    _updateTitle();
  }

  Q_SLOT void onRendererChanged(int index) { _canvas.setRendererType(_rendererSelect.itemData(index).toInt()); }
  Q_SLOT void onLimitFpsChanged(int value) { _timer.setInterval(value ? 1000 / 120 : 0); }

  Q_SLOT void onTimer() {
    _time += 2.0;
    _updateTitle();
    _canvas.updateCanvas(true);
  }

  void onRenderB2D(BLContext& ctx) noexcept {
    ctx.setFillStyle(BLRgba32(0xFF000000));
    ctx.fillAll();

    double kMarginSize = 7;
    double kSquareSize = 45;
    double kFullSize = kSquareSize + kMarginSize * 2.0;
    double kHalfSize = kFullSize / 2.0;

    int w = (_canvas.width() + kFullSize - 1) / kFullSize;
    int h = (_canvas.height() + kFullSize - 1) / kFullSize;

    int count = w * h;
    _count = count;

    double ix = 0;
    double iy = 0;

    double start = 0;
    double now = _time;
    double PI = 3.14159265359;

    BLGradient gr;
    gr.setType(BL_GRADIENT_TYPE_LINEAR);

    for (int i = 0; i < count; i++) {
      double x = ix * kFullSize;
      double y = iy * kFullSize;

      double dur = (now - start) + (i * 50);
      double pos = fmod(dur, 3000.0) / 3000.0;
      double bouncePos = blAbs(pos * 2 - 1);
      double r = (bouncePos * 50 + 50) / 100;
      double b = ((1 - bouncePos) * 50) / 100;

      double rotation = pos * (PI * 2);
      double radius = bouncePos * 25;

      ctx.save();
      ctx.rotate(rotation, x + kHalfSize, y + kHalfSize);
      ctx.translate(x, y);

      gr.resetStops();
      gr.addStop(0, BLRgba32(0xFFFF7F00u));
      gr.addStop(1, BLRgba32(int(r * 255), 0, int(b * 255)));
      gr.setValues(BLLinearGradientValues(0, kMarginSize, 0, kMarginSize + kSquareSize));

      ctx.setFillStyle(gr);
      ctx.fillRoundRect(kMarginSize, kMarginSize, kSquareSize, kSquareSize, radius, radius);
      ctx.restore();

      if (++ix >= w) { ix = 0; iy++; }
    }
  }

  void onRenderQt(QPainter& ctx) noexcept {
    ctx.fillRect(0, 0, _canvas.width(), _canvas.height(), QColor(0, 0, 0));

    ctx.setRenderHint(QPainter::Antialiasing, true);
    ctx.setPen(Qt::NoPen);

    double kMarginSize = 7;
    double kSquareSize = 45;
    double kFullSize = kSquareSize + kMarginSize * 2.0;
    double kHalfSize = kFullSize / 2.0;

    int w = (_canvas.width() + kFullSize - 1) / kFullSize;
    int h = (_canvas.height() + kFullSize - 1) / kFullSize;

    int count = w * h;
    _count = count;

    double ix = 0;
    double iy = 0;

    double start = 0;
    double now = _time;

    for (int i = 0; i < count; i++) {
      double x = ix * kFullSize;
      double y = iy * kFullSize;

      double dur = (now - start) + (i * 50);
      double pos = fmod(dur, 3000.0) / 3000.0;
      double bouncePos = blAbs(pos * 2 - 1);
      double r = (bouncePos * 50 + 50) / 100;
      double b = ((1 - bouncePos) * 50) / 100;

      double rotation = pos * 360;
      double radius = bouncePos * 25;

      QMatrix m;
      m.translate(x + kHalfSize, y + kHalfSize);
      m.rotate(rotation);
      m.translate(-x - kHalfSize, -y - kHalfSize);

      ctx.save();
      ctx.setMatrix(m);
      ctx.translate(x, y);

      QLinearGradient gr(0, kMarginSize, 0, kMarginSize + kSquareSize);
      gr.setColorAt(0, QColor(qRgb(255, 127, 0)));
      gr.setColorAt(1, QColor(qRgb(r * 255, 0, int(b * 255))));

      ctx.setBrush(QBrush(gr));
      ctx.drawRoundedRect(kMarginSize, kMarginSize, kSquareSize, kSquareSize, radius, radius);
      ctx.restore();

      if (++ix >= w) { ix = 0; iy++; }
    }
  }

  void _updateTitle() {
    char buf[256];
    snprintf(buf, 256, "Bounces Sample [%dx%d] [%d objects] [%.1f FPS]",
      _canvas.width(),
      _canvas.height(),
      _count,
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

#include "bl-qt-bounces.moc"

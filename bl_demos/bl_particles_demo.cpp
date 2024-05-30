#include <stdlib.h>
#include <cmath>
#include <vector>

#include "bl_qt_headers.h"
#include "bl_qt_canvas.h"

struct Particle {
  BLPoint p;
  BLPoint v;
  int age;
  int category;
};

class MainWindow : public QWidget {
  Q_OBJECT

public:
  QTimer _timer;
  QComboBox _rendererSelect;
  QCheckBox _limitFpsCheck;
  QCheckBox _colorsCheck;
  QSlider _countSlider;
  QSlider _rotationSlider;
  QBLCanvas _canvas;

  BLRandom _rnd;
  std::vector<Particle> _particles;

  bool _animate = true;
  int maxAge = 650;
  double radiusScale = 6;

  enum { kCategoryCount = 7 };
  BLRgba32 colors[kCategoryCount] = {
    BLRgba32(0xFF4F00FF),
    BLRgba32(0xFFFF004F),
    BLRgba32(0xFFFF7F00),
    BLRgba32(0xFFFF3F9F),
    BLRgba32(0xFF7F4FFF),
    BLRgba32(0xFFFF9F3F),
    BLRgba32(0xFFAF3F00)
  };

  MainWindow() {
    QVBoxLayout* vBox = new QVBoxLayout();
    vBox->setContentsMargins(0, 0, 0, 0);
    vBox->setSpacing(0);

    QGridLayout* grid = new QGridLayout();
    grid->setContentsMargins(5, 5, 5, 5);
    grid->setSpacing(5);

    QBLCanvas::initRendererSelectBox(&_rendererSelect);
    _limitFpsCheck.setText(QLatin1String("Limit FPS"));
    _colorsCheck.setText(QLatin1String("Colors"));

    _countSlider.setMinimum(0);
    _countSlider.setMaximum(5000);
    _countSlider.setValue(500);
    _countSlider.setOrientation(Qt::Horizontal);

    _rotationSlider.setMinimum(0);
    _rotationSlider.setMaximum(1000);
    _rotationSlider.setValue(100);
    _rotationSlider.setOrientation(Qt::Horizontal);

    connect(&_rendererSelect, SIGNAL(activated(int)), SLOT(onRendererChanged(int)));
    connect(&_limitFpsCheck, SIGNAL(stateChanged(int)), SLOT(onLimitFpsChanged(int)));

    grid->addWidget(new QLabel("Renderer:"), 0, 0);
    grid->addWidget(&_rendererSelect, 0, 1);
    grid->addWidget(&_colorsCheck, 0, 2);
    grid->addItem(new QSpacerItem(0, 0, QSizePolicy::Expanding), 0, 3);
    grid->addWidget(&_limitFpsCheck, 0, 4, Qt::AlignRight);

    grid->addWidget(new QLabel("Count:"), 1, 0, Qt::AlignRight);
    grid->addWidget(&_countSlider, 1, 1, 1, 5);

    grid->addWidget(new QLabel("Rotation:"), 2, 0, Qt::AlignRight);
    grid->addWidget(&_rotationSlider, 2, 1, 1, 5);

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
    _rnd.reset(1234);
    _limitFpsCheck.setChecked(true);
    _updateTitle();
  }

  Q_SLOT void onToggleAnimate() { _animate = !_animate; }
  Q_SLOT void onRendererChanged(int index) { _canvas.setRendererType(_rendererSelect.itemData(index).toInt()); }
  Q_SLOT void onLimitFpsChanged(int value) { _timer.setInterval(value ? 1000 / 120 : 0); }

  Q_SLOT void onTimer() {
    if (_animate) {
      size_t i = 0;
      size_t j = 0;
      size_t count = _particles.size();

      double rot = double(_rotationSlider.value()) * 0.02 / 1000;
      double PI = 3.14159265359;
      BLMatrix2D m = BLMatrix2D::makeRotation(rot);

      while (i < count) {
        Particle& p = _particles[i++];
        p.p += p.v;
        p.v = m.mapPoint(p.v);
        if (++p.age >= maxAge)
          continue;
        _particles[j++] = p;
      }
      _particles.resize(j);

      size_t maxParticles = size_t(_countSlider.value());
      size_t n = size_t(_rnd.nextDouble() * maxParticles / 60 + 0.95);

      for (i = 0; i < n; i++) {
        if (_particles.size() >= maxParticles)
          break;

        double angle = _rnd.nextDouble() * PI * 2.0;
        double speed = blMax(_rnd.nextDouble() * 2.0, 0.05);
        double aSin = std::sin(angle);
        double aCos = std::cos(angle);

        Particle part;
        part.p.reset();
        part.v.reset(aCos * speed, aSin * speed);
        part.age = int(blMin(_rnd.nextDouble(), 0.5) * maxAge);
        part.category = int(_rnd.nextDouble() * kCategoryCount);
        _particles.push_back(part);
      }
    }

    _canvas.updateCanvas(true);
    _updateTitle();
  }

  void onRenderB2D(BLContext& ctx) noexcept {
    ctx.fillAll(BLRgba32(0xFF000000u));

    double cx = _canvas.width() / 2;
    double cy = _canvas.height() / 2;

    if (_colorsCheck.isChecked()) {
      BLPath paths[kCategoryCount];

      for (Particle& part : _particles) {
        paths[part.category].addCircle(BLCircle(cx + part.p.x, cy + part.p.y, double(maxAge - part.age) / double(maxAge) * radiusScale));
      }

      ctx.setCompOp(BL_COMP_OP_PLUS);
      for (size_t i = 0; i < kCategoryCount; i++) {
        ctx.fillPath(paths[i], colors[i]);
      }
    }
    else {
      BLPath path;
      for (Particle& part : _particles) {
        path.addCircle(BLCircle(cx + part.p.x, cy + part.p.y, double(maxAge - part.age) / double(maxAge) * radiusScale));
      }
      ctx.fillPath(path, BLRgba32(0xFFFFFFFFu));
    }
  }

  void onRenderQt(QPainter& ctx) noexcept {
    ctx.fillRect(0, 0, _canvas.width(), _canvas.height(), QColor(0, 0, 0));
    ctx.setRenderHint(QPainter::Antialiasing, true);

    double cx = _canvas.width() / 2;
    double cy = _canvas.height() / 2;

    if (_colorsCheck.isChecked()) {
      QPainterPath paths[kCategoryCount];

      for (Particle& part : _particles) {
        double r = double(maxAge - part.age) / double(maxAge) * radiusScale;
        double d = r * 2.0;
        paths[part.category].addEllipse(cx + part.p.x - r, cy + part.p.y - r, d, d);
      }

      ctx.setCompositionMode(QPainter::CompositionMode_Plus);
      for (size_t i = 0; i < kCategoryCount; i++) {
        paths[i].setFillRule(Qt::WindingFill);
        ctx.fillPath(paths[i], QBrush(blRgbaToQColor(colors[i])));
      }
    }
    else {
      QPainterPath p;
      p.setFillRule(Qt::WindingFill);

      for (Particle& part : _particles) {
        double r = double(maxAge - part.age) / double(maxAge) * radiusScale;
        double d = r * 2.0;
        p.addEllipse(cx + part.p.x - r, cy + part.p.y - r, d, d);
      }

      ctx.fillPath(p, QBrush(QColor(qRgb(255, 255, 255))));
    }
  }

  void _updateTitle() {
    char buf[256];
    snprintf(buf, 256, "Particles [%dx%d] [Count=%d] [RenderTime=%.2fms FPS=%.1f]",
      _canvas.width(),
      _canvas.height(),
      int(_particles.size()),
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

#include "bl_particles_demo.moc"

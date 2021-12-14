#include <QtGui>
#include <QtWidgets>

#include <blend2d.h>
#include "qblcanvas.h"
#include "bl-qt-static.h"

class MainWindow : public QWidget {
  Q_OBJECT
public:
  // Widgets.
  QSlider _xRadiusSlider;
  QSlider _yRadiusSlider;
  QSlider _angleSlider;
  QCheckBox _largeArcFlag;
  QCheckBox _sweepArcFlag;
  QLabel _bottomText;
  QBLCanvas _canvas;

  // Canvas data.
  BLGradient _gradient;
  BLPoint _pts[2];
  uint32_t _gradientType;
  uint32_t _gradientExtendMode;
  size_t _numPoints;
  size_t _closestVertex;
  size_t _grabbedVertex;
  int _grabbedX, _grabbedY;

  MainWindow()
    : _closestVertex(SIZE_MAX),
      _grabbedVertex(SIZE_MAX),
      _grabbedX(0),
      _grabbedY(0) {

    setWindowTitle(QLatin1String("SVG Elliptic Arcs"));

    QVBoxLayout* vBox = new QVBoxLayout();
    vBox->setContentsMargins(0, 0, 0, 0);
    vBox->setSpacing(0);

    QGridLayout* grid = new QGridLayout();
    grid->setContentsMargins(5, 5, 5, 5);
    grid->setSpacing(5);

    _xRadiusSlider.setOrientation(Qt::Horizontal);
    _xRadiusSlider.setMinimum(1);
    _xRadiusSlider.setMaximum(500);
    _xRadiusSlider.setSliderPosition(131);

    _yRadiusSlider.setOrientation(Qt::Horizontal);
    _yRadiusSlider.setMinimum(1);
    _yRadiusSlider.setMaximum(500);
    _yRadiusSlider.setSliderPosition(143);

    _angleSlider.setOrientation(Qt::Horizontal);
    _angleSlider.setMinimum(-360);
    _angleSlider.setMaximum(360);
    _angleSlider.setSliderPosition(0);

    _largeArcFlag.setText(QLatin1String("Large Arc Flag"));
    _sweepArcFlag.setText(QLatin1String("Sweep Arc Flag"));

    _bottomText.setTextInteractionFlags(Qt::TextSelectableByMouse);
    _bottomText.setMargin(5);

    connect(&_xRadiusSlider, SIGNAL(valueChanged(int)), SLOT(onParameterChanged(int)));
    connect(&_yRadiusSlider, SIGNAL(valueChanged(int)), SLOT(onParameterChanged(int)));
    connect(&_angleSlider, SIGNAL(valueChanged(int)), SLOT(onParameterChanged(int)));
    connect(&_largeArcFlag, SIGNAL(stateChanged(int)), SLOT(onParameterChanged(int)));
    connect(&_sweepArcFlag, SIGNAL(stateChanged(int)), SLOT(onParameterChanged(int)));

    _canvas.onRenderB2D = std::bind(&MainWindow::onRender, this, std::placeholders::_1);
    _canvas.onMouseEvent = std::bind(&MainWindow::onMouseEvent, this, std::placeholders::_1);

    grid->addWidget(new QLabel("X Radius:"), 0, 0, Qt::AlignRight);
    grid->addWidget(&_xRadiusSlider, 0, 1);
    grid->addWidget(&_largeArcFlag, 0, 2);

    grid->addWidget(new QLabel("Y Radius:"), 1, 0, Qt::AlignRight);
    grid->addWidget(&_yRadiusSlider, 1, 1);
    grid->addWidget(&_sweepArcFlag, 1, 2);

    grid->setSpacing(5);
    grid->addWidget(new QLabel("Angle:"), 2, 0, Qt::AlignRight);
    grid->addWidget(&_angleSlider, 2, 1, 1, 2);

    vBox->addItem(grid);
    vBox->addWidget(&_canvas);
    vBox->addWidget(&_bottomText);
    setLayout(vBox);

    onInit();
  }

  void keyPressEvent(QKeyEvent *event) override {}

  void onInit() {
    _pts[0].reset(124, 180);
    _pts[1].reset(296, 284);
    _numPoints = 2;
    _gradientType = BL_GRADIENT_TYPE_LINEAR;
    _gradientExtendMode = BL_EXTEND_MODE_PAD;
    _gradient.addStop(0.0, BLRgba32(0xFF000000u));
    _gradient.addStop(1.0, BLRgba32(0xFFFFFFFFu));
  }

  size_t getClosestVertex(BLPoint p, double maxDistance) noexcept {
    size_t closestIndex = SIZE_MAX;
    double closestDistance = std::numeric_limits<double>::max();
    for (size_t i = 0; i < _numPoints; i++) {
      double d = std::hypot(_pts[i].x - p.x, _pts[i].y - p.y);
      if (d < closestDistance && d < maxDistance) {
        closestIndex = i;
        closestDistance = d;
      }
    }
    return closestIndex;
  }

  void onMouseEvent(QMouseEvent* event) {
    double mx = event->x();
    double my = event->y();

    if (event->type() == QEvent::MouseButtonPress) {
      if (event->button() == Qt::LeftButton) {
        if (_closestVertex != SIZE_MAX) {
          _grabbedVertex = _closestVertex;
          _grabbedX = mx;
          _grabbedY = my;
          _canvas.updateCanvas();
        }
      }
    }

    if (event->type() == QEvent::MouseButtonRelease) {
      if (event->button() == Qt::LeftButton) {
        if (_grabbedVertex != SIZE_MAX) {
          _grabbedVertex = SIZE_MAX;
          _canvas.updateCanvas();
        }
      }
    }

    if (event->type() == QEvent::MouseMove) {
      if (_grabbedVertex == SIZE_MAX) {
        _closestVertex = getClosestVertex(BLPoint(mx, my), 5);
        _canvas.updateCanvas();
      }
      else {
        _pts[_grabbedVertex] = BLPoint(mx, my);
        _canvas.updateCanvas();
      }
    }
  }

  Q_SLOT void onParameterChanged(int value) {
    _canvas.updateCanvas();
  }

  void onRender(BLContext& ctx) {
    ctx.setFillStyle(BLRgba32(0xFF000000u));
    ctx.fillAll();

    BLPoint radius(_xRadiusSlider.value(), _yRadiusSlider.value());
    BLPoint start(_pts[0]);
    BLPoint end(_pts[1]);

    double PI = 3.14159265359;
    double angle = (double(_angleSlider.value()) / 180.0) * PI;

    bool largeArcFlag = _largeArcFlag.isChecked();
    bool sweepArcFlag = _sweepArcFlag.isChecked();

    // Render all arcs before rendering the one that is selected.
    BLPath p;
    p.moveTo(start);
    p.ellipticArcTo(radius, angle, false, false, end);
    p.moveTo(start);
    p.ellipticArcTo(radius, angle, false, true, end);
    p.moveTo(start);
    p.ellipticArcTo(radius, angle, true, false, end);
    p.moveTo(start);
    p.ellipticArcTo(radius, angle, true, true, end);

    ctx.setStrokeStyle(BLRgba32(0x40FFFFFFu));
    ctx.strokePath(p);

    // Render elliptic arc based on the given parameters.
    p.clear();
    p.moveTo(start);
    p.ellipticArcTo(radius, angle, largeArcFlag, sweepArcFlag, end);

    ctx.setStrokeStyle(BLRgba32(0xFFFFFFFFu));
    ctx.strokePath(p);

    // Render all points of the path (as the arc was split into segments).
    renderPathPoints(ctx, p, BLRgba32(0xFF808080));

    // Render the rest of the UI (draggable points).
    for (size_t i = 0; i < _numPoints; i++) {
      ctx.setFillStyle(i == _closestVertex ? BLRgba32(0xFF00FFFFu) : BLRgba32(0xFF007FFFu));
      ctx.fillCircle(_pts[i].x, _pts[i].y, 2.5);
    }

    char buf[256];
    snprintf(buf, 256, "<path d=\"M%g %g A%g %g %g %d %d %g %g\" />", start.x, start.y, radius.x, radius.y, angle / PI * 180, largeArcFlag, sweepArcFlag, end.x, end.y);
    _bottomText.setText(QString::fromUtf8(buf));
  }

  void renderPathPoints(BLContext& ctx, const BLPath& path, BLRgba32 color) noexcept {
    size_t count = path.size();
    const BLPoint* vtx = path.vertexData();

    ctx.setFillStyle(color);
    for (size_t i = 0; i < count; i++) {
      if (!std::isfinite(vtx[i].x))
        continue;
      ctx.fillCircle(vtx[i].x, vtx[i].y, 2.0);
    }
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

#include "bl-qt-ellipticarc.moc"

#include <blend2d.h>

#include "bl_qt_headers.h"
#include "bl_qt_canvas.h"

class MainWindow : public QWidget {
  Q_OBJECT
public:
  // Widgets.
  QComboBox _gradientTypeSelect;
  QComboBox _extendModeSelect;
  QSlider _parameterSlider;
  QBLCanvas _canvas;

  // Canvas data.
  BLGradient _gradient;
  BLPoint _pts[2];
  BLGradientType _gradientType;
  BLExtendMode _gradientExtendMode;
  size_t _numPoints;
  size_t _closestVertex;
  size_t _grabbedVertex;
  int _grabbedX, _grabbedY;

  MainWindow()
    : _closestVertex(SIZE_MAX),
      _grabbedVertex(SIZE_MAX),
      _grabbedX(0),
      _grabbedY(0) {

    setWindowTitle(QLatin1String("Gradients Sample"));

    QVBoxLayout* vBox = new QVBoxLayout();
    vBox->setContentsMargins(0, 0, 0, 0);
    vBox->setSpacing(0);

    QGridLayout* grid = new QGridLayout();
    grid->setContentsMargins(5, 5, 5, 5);
    grid->setSpacing(5);

    _gradientTypeSelect.addItem("Linear", QVariant(int(BL_GRADIENT_TYPE_LINEAR)));
    _gradientTypeSelect.addItem("Radial", QVariant(int(BL_GRADIENT_TYPE_RADIAL)));
    _gradientTypeSelect.addItem("Conical", QVariant(int(BL_GRADIENT_TYPE_CONICAL)));
    connect(&_gradientTypeSelect, SIGNAL(activated(int)), SLOT(onGradientTypeChanged(int)));

    _extendModeSelect.addItem("Pad", QVariant(int(BL_EXTEND_MODE_PAD)));
    _extendModeSelect.addItem("Repeat", QVariant(int(BL_EXTEND_MODE_REPEAT)));
    _extendModeSelect.addItem("Reflect", QVariant(int(BL_EXTEND_MODE_REFLECT)));
    connect(&_extendModeSelect, SIGNAL(activated(int)), SLOT(onExtendModeChanged(int)));

    _parameterSlider.setOrientation(Qt::Horizontal);
    _parameterSlider.setMinimum(1);
    _parameterSlider.setMaximum(500);
    _parameterSlider.setSliderPosition(250);
    connect(&_parameterSlider, SIGNAL(valueChanged(int)), SLOT(onParameterChanged(int)));

    _canvas.onRenderB2D = std::bind(&MainWindow::onRender, this, std::placeholders::_1);
    _canvas.onMouseEvent = std::bind(&MainWindow::onMouseEvent, this, std::placeholders::_1);

    QPushButton* colorsButton = new QPushButton("Colors");
    connect(colorsButton, SIGNAL(clicked()), SLOT(onRandomizeColors()));

    QPushButton* randomizeButton = new QPushButton("Random");
    connect(randomizeButton, SIGNAL(clicked()), SLOT(onRandomizeVertices()));

    grid->addWidget(new QLabel("Gradient:"), 0, 0, Qt::AlignRight);
    grid->addWidget(&_gradientTypeSelect, 0, 1);
    grid->addItem(new QSpacerItem(0, 10), 0, 2);
    grid->addWidget(new QLabel("Extend Mode:"), 0, 3);
    grid->addWidget(&_extendModeSelect, 0, 4);
    grid->addItem(new QSpacerItem(0, 0, QSizePolicy::Expanding), 0, 5);
    grid->addWidget(colorsButton, 0, 6);
    grid->addWidget(randomizeButton, 0, 7);

    grid->addWidget(new QLabel("Radius:"), 1, 0, Qt::AlignRight);
    grid->addWidget(&_parameterSlider, 1, 1, 1, 7);

    vBox->addItem(grid);
    vBox->addWidget(&_canvas);
    setLayout(vBox);

    onInit();
  }

  void keyPressEvent(QKeyEvent *event) override {}

  void onInit() {
    _pts[0].reset(100, 80);
    _pts[1].reset(350, 150);
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

  Q_SLOT void onGradientTypeChanged(int index) {
    _numPoints = index == BL_GRADIENT_TYPE_CONICAL ? 1 : 2;
    _gradientType = (BLGradientType)index;
    _canvas.updateCanvas();
  }

  Q_SLOT void onExtendModeChanged(int index) {
    _gradientExtendMode = (BLExtendMode)index;
    _canvas.updateCanvas();
  }

  Q_SLOT void onParameterChanged(int value) {
    _canvas.updateCanvas();
  }

  Q_SLOT void onRandomizeColors() {
    auto randomColor = []() { return BLRgba32(rand() % 256, rand() % 256, rand() % 256); };

    _gradient.resetStops();
    _gradient.addStop(0.0, randomColor());
    _gradient.addStop(0.5, randomColor());
    _gradient.addStop(1.0, randomColor());
    _canvas.updateCanvas();
  }

  Q_SLOT void onRandomizeVertices() {
    _pts[0].x = (double(rand() % 65536) / 65535.0) * (_canvas.width()  - 1) + 0.5;
    _pts[0].y = (double(rand() % 65536) / 65535.0) * (_canvas.height() - 1) + 0.5;
    _pts[1].x = (double(rand() % 65536) / 65535.0) * (_canvas.width()  - 1) + 0.5;
    _pts[1].y = (double(rand() % 65536) / 65535.0) * (_canvas.height() - 1) + 0.5;
    _canvas.updateCanvas();
  }

  void onRender(BLContext& ctx) {
    _gradient.setType(_gradientType);
    _gradient.setExtendMode(_gradientExtendMode);

    if (_gradientType == BL_GRADIENT_TYPE_LINEAR) {
      _gradient.setValues(BLLinearGradientValues { _pts[0].x, _pts[0].y, _pts[1].x, _pts[1].y });
    }
    else if (_gradientType == BL_GRADIENT_TYPE_RADIAL) {
      _gradient.setValues(BLRadialGradientValues { _pts[0].x, _pts[0].y, _pts[1].x, _pts[1].y, double(_parameterSlider.value()) });
    }
    else {
      _gradient.setValues(BLConicalGradientValues { _pts[0].x, _pts[0].y, double(_parameterSlider.value()) });
    }

    ctx.setFillStyle(_gradient);
    ctx.fillAll();

    for (size_t i = 0; i < _numPoints; i++) {
      ctx.setFillStyle(i == _closestVertex ? BLRgba32(0xFF00FFFFu) : BLRgba32(0xFF007FFFu));
      ctx.fillCircle(_pts[i].x, _pts[i].y, 2);
    }
  }
};

int main(int argc, char *argv[]) {
  QApplication app(argc, argv);
  MainWindow win;

  win.setMinimumSize(QSize(500, 400));
  win.show();

  return app.exec();
}

#include "bl_qt_gradients.moc"

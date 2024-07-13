#include <blend2d.h>

#include "bl_qt_headers.h"
#include "bl_qt_canvas.h"

class MainWindow : public QWidget {
  Q_OBJECT
public:
  // Widgets.
  QComboBox _rendererSelect;
  QComboBox _gradientTypeSelect;
  QComboBox _extendModeSelect;
  QSlider _parameterSlider1;
  QSlider _parameterSlider2;
  QLabel _parameterLabel1;
  QLabel _parameterLabel2;
  QSlider _stopSliders[9];
  QCheckBox _controlCheck;
  QCheckBox _ditherCheck;
  QBLCanvas _canvas;

  // Canvas data.
  BLPoint _pts[2] {};
  BLGradientType _gradientType = BL_GRADIENT_TYPE_LINEAR;
  BLExtendMode _gradientExtendMode = BL_EXTEND_MODE_PAD;
  size_t _numPoints = 2;
  size_t _closestVertex = SIZE_MAX;
  size_t _grabbedVertex = SIZE_MAX;
  int _grabbedX = 0;
  int _grabbedY = 0;

  MainWindow() {
    setWindowTitle(QLatin1String("Gradients"));

    QVBoxLayout* vBox = new QVBoxLayout();
    vBox->setContentsMargins(0, 0, 0, 0);
    vBox->setSpacing(0);

    QGridLayout* grid = new QGridLayout();
    grid->setContentsMargins(5, 5, 5, 5);
    grid->setSpacing(5);

    QBLCanvas::initRendererSelectBox(&_rendererSelect);
    connect(&_rendererSelect, SIGNAL(activated(int)), SLOT(onRendererChanged(int)));

    _gradientTypeSelect.addItem("Linear", QVariant(int(BL_GRADIENT_TYPE_LINEAR)));
    _gradientTypeSelect.addItem("Radial", QVariant(int(BL_GRADIENT_TYPE_RADIAL)));
    _gradientTypeSelect.addItem("Conic", QVariant(int(BL_GRADIENT_TYPE_CONIC)));
    connect(&_gradientTypeSelect, SIGNAL(activated(int)), SLOT(onGradientTypeChanged(int)));

    _extendModeSelect.addItem("Pad", QVariant(int(BL_EXTEND_MODE_PAD)));
    _extendModeSelect.addItem("Repeat", QVariant(int(BL_EXTEND_MODE_REPEAT)));
    _extendModeSelect.addItem("Reflect", QVariant(int(BL_EXTEND_MODE_REFLECT)));
    connect(&_extendModeSelect, SIGNAL(activated(int)), SLOT(onExtendModeChanged(int)));

    _parameterSlider1.setOrientation(Qt::Horizontal);
    _parameterSlider1.setMinimum(0);
    _parameterSlider1.setMaximum(720);
    _parameterSlider1.setSliderPosition(360);
    connect(&_parameterSlider1, SIGNAL(valueChanged(int)), SLOT(onParameterChanged(int)));

    _parameterSlider2.setOrientation(Qt::Horizontal);
    _parameterSlider2.setMinimum(0);
    _parameterSlider2.setMaximum(720);
    _parameterSlider2.setSliderPosition(0);
    connect(&_parameterSlider2, SIGNAL(valueChanged(int)), SLOT(onParameterChanged(int)));

    _controlCheck.setText("Control");
    _controlCheck.setChecked(true);
    connect(&_controlCheck, SIGNAL(stateChanged(int)), SLOT(onParameterChanged(int)));

    _ditherCheck.setText("Dither");
    connect(&_ditherCheck, SIGNAL(stateChanged(int)), SLOT(onParameterChanged(int)));

    const uint32_t initialColors[3] = {
      0xFF000000,
      0xFFFF0000,
      0xFFFFFFFF
    };

    for (uint32_t stopId = 0; stopId < 3; stopId++) {
      uint32_t color = initialColors[stopId];
      for (uint32_t component = 0; component < 3; component++, color <<= 8) {
        uint32_t sliderId = stopId * 3 + component;
        _stopSliders[sliderId].setOrientation(Qt::Horizontal);
        _stopSliders[sliderId].setMinimum(0);
        _stopSliders[sliderId].setMaximum(255);
        _stopSliders[sliderId].setSliderPosition((color >> 16) & 0xFF);

        static const char channels[] = "R:\0G:\0B:\0";
        grid->addWidget(new QLabel(QString::fromLatin1(channels + component * 3, 2)), component, stopId * 2 + 2);
        grid->addWidget(&_stopSliders[sliderId], component, stopId * 2 + 3);
        connect(&_stopSliders[sliderId], SIGNAL(valueChanged(int)), SLOT(onParameterChanged(int)));
      }
    }

    QPushButton* randomizeButton = new QPushButton("Random");
    connect(randomizeButton, SIGNAL(clicked()), SLOT(onRandomizeVertices()));

    grid->addWidget(new QLabel("Renderer:"), 0, 0, Qt::AlignRight);
    grid->addWidget(&_rendererSelect, 0, 1);
    grid->addWidget(new QLabel("Gradient:"), 1, 0, Qt::AlignRight);
    grid->addWidget(&_gradientTypeSelect, 1, 1);
    grid->addItem(new QSpacerItem(0, 10), 0, 2);
    grid->addWidget(new QLabel("Extend Mode:"), 2, 0);
    grid->addWidget(&_extendModeSelect, 2, 1);

    grid->addItem(new QSpacerItem(0, 0, QSizePolicy::Expanding), 0, 7);
    grid->addWidget(randomizeButton, 0, 8);

    grid->addWidget(&_parameterLabel1, 3, 0, Qt::AlignRight);
    grid->addWidget(&_parameterSlider1, 3, 1, 1, 8);

    grid->addWidget(&_parameterLabel2, 4, 0, Qt::AlignRight);
    grid->addWidget(&_parameterSlider2, 4, 1, 1, 8);

    grid->addWidget(&_controlCheck, 1, 8);
    grid->addWidget(&_ditherCheck, 2, 8);

    vBox->addItem(grid);
    vBox->addWidget(&_canvas);
    setLayout(vBox);

    _canvas.onRenderB2D = std::bind(&MainWindow::onRenderB2D, this, std::placeholders::_1);
    _canvas.onRenderQt = std::bind(&MainWindow::onRenderQt, this, std::placeholders::_1);
    _canvas.onMouseEvent = std::bind(&MainWindow::onMouseEvent, this, std::placeholders::_1);

    onInit();
    updateLabels();
  }

  void updateLabels() {
    switch (_gradientType) {
      case BL_GRADIENT_TYPE_LINEAR:
        _parameterLabel1.setText("(Unused)");
        _parameterLabel2.setText("(Unused)");
        break;

      case BL_GRADIENT_TYPE_RADIAL:
        _parameterLabel1.setText("Center Rad");
        _parameterLabel2.setText("Focal Rad");
        break;

      case BL_GRADIENT_TYPE_CONIC:
        _parameterLabel1.setText("Angle");
        _parameterLabel2.setText("Repeat");
        break;
    }
  }

  void keyPressEvent(QKeyEvent *event) override {}

  void onInit() {
    _pts[0].reset(350, 300);
    _pts[1].reset(200, 150);
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

  double sliderAngle(double scale) {
    return double(_parameterSlider1.value()) / 720.0 * scale;
  }

  void onMouseEvent(QMouseEvent* event) {
    double mx = event->position().x();
    double my = event->position().y();

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

  Q_SLOT void onRendererChanged(int index) {
    _canvas.setRendererType(_rendererSelect.itemData(index).toInt());
  }

  Q_SLOT void onGradientTypeChanged(int index) {
    _numPoints = index == BL_GRADIENT_TYPE_CONIC ? 1 : 2;
    _gradientType = BLGradientType(index);
    updateLabels();
    _canvas.updateCanvas();
  }

  Q_SLOT void onExtendModeChanged(int index) {
    _gradientExtendMode = (BLExtendMode)index;
    _canvas.updateCanvas();
  }

  Q_SLOT void onParameterChanged(int value) {
    _canvas.updateCanvas();
  }

  Q_SLOT void onRandomizeVertices() {
    _pts[0].x = (double(rand() % 65536) / 65535.0) * (_canvas.imageWidth()  - 1) + 0.5;
    _pts[0].y = (double(rand() % 65536) / 65535.0) * (_canvas.imageHeight() - 1) + 0.5;
    _pts[1].x = (double(rand() % 65536) / 65535.0) * (_canvas.imageWidth()  - 1) + 0.5;
    _pts[1].y = (double(rand() % 65536) / 65535.0) * (_canvas.imageHeight() - 1) + 0.5;
    _canvas.updateCanvas();
  }

  void onRenderB2D(BLContext& ctx) noexcept {
    if (_ditherCheck.isChecked()) {
      ctx.setGradientQuality(BL_GRADIENT_QUALITY_DITHER);
    }

    BLGradient gradient;
    gradient.setType(_gradientType);
    gradient.setExtendMode(_gradientExtendMode);
    gradient.resetStops();

    constexpr double offsets[] = { 0.0, 0.5, 1.0 };
    for (uint32_t stopId = 0; stopId < 3; stopId++) {
      uint32_t r = uint32_t(_stopSliders[stopId * 3 + 0].value());
      uint32_t g = uint32_t(_stopSliders[stopId * 3 + 1].value());
      uint32_t b = uint32_t(_stopSliders[stopId * 3 + 2].value());
      gradient.addStop(offsets[stopId], BLRgba32(r, g, b));
    }

    if (_gradientType == BL_GRADIENT_TYPE_LINEAR) {
      gradient.setValues(
        BLLinearGradientValues(
          _pts[0].x,
          _pts[0].y,
          _pts[1].x,
          _pts[1].y));
    }
    else if (_gradientType == BL_GRADIENT_TYPE_RADIAL) {
      gradient.setValues(
        BLRadialGradientValues(
          _pts[0].x,
          _pts[0].y,
          _pts[1].x,
          _pts[1].y,
          double(_parameterSlider1.value()),
          double(_parameterSlider2.value())));
    }
    else {
      gradient.setValues(
        BLConicGradientValues(
          _pts[0].x,
          _pts[0].y,
          sliderAngle(3.14159265358979323846 * 2.0),
          double(_parameterSlider2.value()) / 100.0 + 1.0));
    }

    ctx.fillAll(gradient);

    if (_controlCheck.isChecked()) {
      for (size_t i = 0; i < _numPoints; i++) {
        ctx.strokeCircle(_pts[i].x, _pts[i].y, 3, i == _closestVertex ? BLRgba32(0xFF00FFFFu) : BLRgba32(0xFF007FFFu));
      }
    }
  }

  void onRenderQt(QPainter& ctx) noexcept {
    ctx.fillRect(0, 0, _canvas.imageWidth(), _canvas.imageHeight(), QColor(255, 0, 0));
    ctx.setRenderHint(QPainter::Antialiasing, true);

    QGradientStops stops;
    const double offsets[] = { 0.0, 0.5, 1.0 };
    for (uint32_t stopId = 0; stopId < 3; stopId++) {
      uint32_t r = uint32_t(_stopSliders[stopId * 3 + 0].value());
      uint32_t g = uint32_t(_stopSliders[stopId * 3 + 1].value());
      uint32_t b = uint32_t(_stopSliders[stopId * 3 + 2].value());
      stops.append(QGradientStop(qreal(offsets[stopId]), QColor(qRgb(r, g, b))));
    }

    QGradient::Spread spread = QGradient::PadSpread;
    if (_gradientExtendMode == BL_EXTEND_MODE_REPEAT) spread = QGradient::RepeatSpread;
    if (_gradientExtendMode == BL_EXTEND_MODE_REFLECT) spread = QGradient::ReflectSpread;

    switch (_gradientType) {
      case BL_GRADIENT_TYPE_LINEAR: {
        QLinearGradient g(_pts[0].x, _pts[0].y, _pts[1].x, _pts[1].y);
        g.setStops(stops);
        g.setSpread(spread);
        ctx.fillRect(0, 0, _canvas.imageWidth(), _canvas.imageHeight(), QBrush(g));
        break;
      }

      case BL_GRADIENT_TYPE_RADIAL: {
        QRadialGradient g(
          qreal(_pts[0].x), qreal(_pts[0].y), qreal(_parameterSlider1.value()),
          qreal(_pts[1].x), qreal(_pts[1].y), qreal(_parameterSlider2.value()));
        g.setStops(stops);
        g.setSpread(spread);
        ctx.fillRect(0, 0, _canvas.imageWidth(), _canvas.imageHeight(), QBrush(g));
        break;
      }

      case BL_GRADIENT_TYPE_CONIC: {
        QConicalGradient g(_pts[0].x, _pts[0].y, sliderAngle(360.0));
        g.setSpread(spread);
        g.setStops(stops);
        ctx.fillRect(0, 0, _canvas.imageWidth(), _canvas.imageHeight(), QBrush(g));
        break;
      }
    }

    if (_controlCheck.isChecked()) {
      for (size_t i = 0; i < _numPoints; i++) {
        QColor color = blRgbaToQColor(i == _closestVertex ? BLRgba32(0xFF00FFFFu) : BLRgba32(0xFF007FFFu));
        ctx.setPen(QPen(color, 1.0f));
        ctx.setBrush(QBrush());
        ctx.drawEllipse(QPointF(_pts[i].x, _pts[i].y), 3, 3);
      }
    }
  }
};

int main(int argc, char *argv[]) {
  QApplication app(argc, argv);
  MainWindow win;

  win.setMinimumSize(QSize(700, 650));
  win.show();

  return app.exec();
}

#include "bl_gradients_demo.moc"

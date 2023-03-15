#include <blend2d.h>

#include "bl_qt_headers.h"
#include "bl_qt_canvas.h"

class MainWindow : public QWidget {
  Q_OBJECT
public:
  // Widgets.
  QComboBox _capTypeSelect;
  QComboBox _joinTypeSelect;
  QSlider _widthSlider;
  QSlider _miterLimitSlider;
  QBLCanvas _canvas;

  // Canvas data.
  BLRandom _prng;
  BLPath _path;
  bool _showControl;
  size_t _closestVertex;
  size_t _grabbedVertex;
  int _grabbedX, _grabbedY;
  BLStrokeOptions _strokeOptions;

  MainWindow()
    : _showControl(true),
      _closestVertex(SIZE_MAX),
      _grabbedVertex(SIZE_MAX),
      _grabbedX(0),
      _grabbedY(0) {

    setWindowTitle(QLatin1String("Stroke Sample"));
    int maxPbWidth = 30;
    int labelWidth = 90;

    QVBoxLayout* vBox = new QVBoxLayout();
    vBox->setContentsMargins(0, 0, 0, 0);
    vBox->setSpacing(0);

    QGridLayout* grid = new QGridLayout();
    grid->setContentsMargins(5, 5, 5, 5);
    grid->setSpacing(5);

    QPushButton* pbA = new QPushButton("A");
    QPushButton* pbB = new QPushButton("B");
    QPushButton* pbC = new QPushButton("C");
    QPushButton* pbD = new QPushButton("D");
    QPushButton* pbE = new QPushButton("E");
    QPushButton* pbF = new QPushButton("F");
    QPushButton* pbX = new QPushButton("X");
    QPushButton* pbY = new QPushButton("Y");
    QPushButton* pbZ = new QPushButton("Z");
    QPushButton* pbRandom = new QPushButton("Random");
    QPushButton* pbDump = new QPushButton(QString::fromLatin1("Dump"));

    pbA->setMaximumWidth(maxPbWidth);
    pbB->setMaximumWidth(maxPbWidth);
    pbC->setMaximumWidth(maxPbWidth);
    pbD->setMaximumWidth(maxPbWidth);
    pbE->setMaximumWidth(maxPbWidth);
    pbF->setMaximumWidth(maxPbWidth);
    pbX->setMaximumWidth(maxPbWidth);
    pbY->setMaximumWidth(maxPbWidth);
    pbZ->setMaximumWidth(maxPbWidth);

    _capTypeSelect.addItem("Butt", QVariant(int(BL_STROKE_CAP_BUTT)));
    _capTypeSelect.addItem("Square", QVariant(int(BL_STROKE_CAP_SQUARE)));
    _capTypeSelect.addItem("Round", QVariant(int(BL_STROKE_CAP_ROUND)));
    _capTypeSelect.addItem("Round-Rev", QVariant(int(BL_STROKE_CAP_ROUND_REV)));
    _capTypeSelect.addItem("Triangle", QVariant(int(BL_STROKE_CAP_TRIANGLE)));
    _capTypeSelect.addItem("Triangle-Rev", QVariant(int(BL_STROKE_CAP_TRIANGLE_REV)));

    _joinTypeSelect.addItem("Miter-Clip", QVariant(int(BL_STROKE_JOIN_MITER_CLIP)));
    _joinTypeSelect.addItem("Miter-Bevel", QVariant(int(BL_STROKE_JOIN_MITER_BEVEL)));
    _joinTypeSelect.addItem("Miter-Round", QVariant(int(BL_STROKE_JOIN_MITER_ROUND)));
    _joinTypeSelect.addItem("Bevel", QVariant(int(BL_STROKE_JOIN_BEVEL)));
    _joinTypeSelect.addItem("Round", QVariant(int(BL_STROKE_JOIN_ROUND)));

    connect(pbA, SIGNAL(clicked()), SLOT(onSetDataA()));
    connect(pbB, SIGNAL(clicked()), SLOT(onSetDataB()));
    connect(pbC, SIGNAL(clicked()), SLOT(onSetDataC()));
    connect(pbD, SIGNAL(clicked()), SLOT(onSetDataD()));
    connect(pbE, SIGNAL(clicked()), SLOT(onSetDataE()));
    connect(pbF, SIGNAL(clicked()), SLOT(onSetDataF()));
    connect(pbX, SIGNAL(clicked()), SLOT(onSetDataX()));
    connect(pbY, SIGNAL(clicked()), SLOT(onSetDataY()));
    connect(pbZ, SIGNAL(clicked()), SLOT(onSetDataZ()));
    connect(pbDump, SIGNAL(clicked()), SLOT(onDumpPath()));
    connect(pbRandom, SIGNAL(clicked()), SLOT(onSetRandom()));
    connect(&_capTypeSelect, SIGNAL(activated(int)), SLOT(onCapTypeUpdate(int)));
    connect(&_joinTypeSelect, SIGNAL(activated(int)), SLOT(onJoinTypeUpdate(int)));

    _widthSlider.setOrientation(Qt::Horizontal);
    _widthSlider.setMinimum(1);
    _widthSlider.setMaximum(400);
    _widthSlider.setSliderPosition(40);
    connect(&_widthSlider, SIGNAL(valueChanged(int)), SLOT(onWidthChanged(int)));

    _miterLimitSlider.setOrientation(Qt::Horizontal);
    _miterLimitSlider.setMinimum(0);
    _miterLimitSlider.setMaximum(1000);
    _miterLimitSlider.setSliderPosition(400);
    connect(&_miterLimitSlider, SIGNAL(valueChanged(int)), SLOT(onMiterLimitChanged(int)));

    _canvas.onRenderB2D = std::bind(&MainWindow::onRender, this, std::placeholders::_1);
    _canvas.onMouseEvent = std::bind(&MainWindow::onMouseEvent, this, std::placeholders::_1);

    grid->addWidget(new QLabel("Stroke Caps:"), 0, 0, Qt::AlignRight);
    grid->addWidget(&_capTypeSelect, 0, 1);
    grid->addWidget(pbRandom, 0, 2);
    grid->addItem(new QSpacerItem(0, 0, QSizePolicy::Expanding), 0, 3);
    grid->addWidget(pbA, 0, 4);
    grid->addWidget(pbB, 0, 5);
    grid->addWidget(pbC, 0, 6);
    grid->addWidget(pbD, 0, 7);
    grid->addWidget(pbE, 0, 8);
    grid->addWidget(pbF, 0, 9);

    grid->addWidget(new QLabel("Stroke Join:"), 1, 0, Qt::AlignRight);
    grid->addWidget(&_joinTypeSelect, 1, 1);
    grid->addWidget(pbDump, 1, 2);
    grid->addItem(new QSpacerItem(0, 0, QSizePolicy::Expanding), 1, 3, 1, 4);
    grid->addWidget(pbX, 1, 7);
    grid->addWidget(pbY, 1, 8);
    grid->addWidget(pbZ, 1, 9);

    grid->addWidget(new QLabel("Width:"), 2, 0, Qt::AlignRight);
    grid->addWidget(&_widthSlider, 2, 1, 1, 10);

    grid->addWidget(new QLabel("Miter Limit:"), 3, 0, Qt::AlignRight);
    grid->addWidget(&_miterLimitSlider, 3, 1, 1, 10);

    vBox->addLayout(grid);
    vBox->addWidget(&_canvas);
    setLayout(vBox);
    onInit();
  }

  void keyPressEvent(QKeyEvent *event) override {
    if (event->key() == Qt::Key_Z) {
      _showControl = !_showControl;
      _canvas.updateCanvas();
    }
  }

  void onInit() {
    _prng.reset(QCoreApplication::applicationPid());
    _strokeOptions.width = _widthSlider.sliderPosition();
    _strokeOptions.miterLimit = 5;

    onSetDataA();
  }

  void onMouseEvent(QMouseEvent* event) {
    if (event->type() == QEvent::MouseButtonPress) {
      if (event->button() == Qt::LeftButton) {
        if (_closestVertex != SIZE_MAX) {
          _grabbedVertex = _closestVertex;
          _grabbedX = event->x();
          _grabbedY = event->y();
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
        _path.getClosestVertex(BLPoint(double(event->x()), double(event->y())), 5, &_closestVertex);;
        _canvas.updateCanvas();
      }
      else {
        double x = event->x();
        double y = event->y();
        _path.setVertexAt(_grabbedVertex, BL_PATH_CMD_PRESERVE, BLPoint(x, y));
        _canvas.updateCanvas();
      }
    }
  }

  Q_SLOT void onSetRandom() {
    double minX = 25;
    double minY = 25;
    double maxX = double(_canvas.width()) - minX;
    double maxY = double(_canvas.height()) - minY;

    auto rx = [&]() { return _prng.nextDouble() * (maxX - minX) + minX; };
    auto ry = [&]() { return _prng.nextDouble() * (maxY - minY) + minY; };

    _path.clear();
    _path.moveTo(rx(), ry());

    double cmd = _prng.nextDouble();
    if (cmd < 0.33) {
      _path.lineTo(rx(), ry());
      _path.lineTo(rx(), ry());
      _path.lineTo(rx(), ry());
    }
    else if (cmd < 0.66) {
      _path.quadTo(rx(), ry(), rx(), ry());
      _path.quadTo(rx(), ry(), rx(), ry());
    }
    else {
      _path.cubicTo(rx(), ry(), rx(), ry(), rx(), ry());
    }

    if (_prng.nextDouble() < 0.5)
      _path.close();

    _canvas.updateCanvas();
  }

  Q_SLOT void onSetDataA() {
    _path.clear();
    _path.moveTo(345, 333);
    _path.cubicTo(308, 3, 33, 352, 512, 244);
    _canvas.updateCanvas();
  }

  Q_SLOT void onSetDataB() {
    _path.clear();
    _path.moveTo(60, 177);
    _path.quadTo(144, 354, 396, 116);
    _path.quadTo(106, 184, 43.4567, 43.3091);
    _canvas.updateCanvas();
  }

  Q_SLOT void onSetDataC() {
    _path.clear();
    _path.moveTo(488, 45);
    _path.cubicTo(22, 331, 26, 27, 493, 338);
    _canvas.updateCanvas();
  }

  Q_SLOT void onSetDataD() {
    _path.clear();
    _path.moveTo(276, 152);
    _path.lineTo(194.576, 54.1927);
    _path.lineTo(114, 239);
    _path.lineTo(526.311, 134.453);
    _canvas.updateCanvas();
  }

  Q_SLOT void onSetDataE() {
    _path.clear();
    _path.moveTo(161, 308);
    _path.cubicTo(237.333, 152.509, 146.849, 108.62, 467.225, 59.9782);
    _path.close();
    _canvas.updateCanvas();
  }

  Q_SLOT void onSetDataF() {
    _path.clear();
    _path.addCircle(BLCircle(280, 190, 140));
    _canvas.updateCanvas();
  }

  Q_SLOT void onSetDataX() {
    _path.clear();
    _path.moveTo(300, 200);
    _path.quadTo(50, 200, 500, 200);
    _canvas.updateCanvas();
  }

  Q_SLOT void onSetDataY() {
    _path.clear();
    _path.moveTo(300, 200);
    _path.cubicTo(50, 200, 500, 200, 350, 200);
    _canvas.updateCanvas();
  }

  Q_SLOT void onSetDataZ() {
    _path.clear();
    _path.moveTo(300, 200);
    _path.lineTo(50, 200);
    _path.lineTo(500, 200);
    _path.lineTo(350, 200);
    _canvas.updateCanvas();
  }

  Q_SLOT void onDumpPath() {
    size_t count = _path.size();
    const BLPoint* vtx = _path.vertexData();
    const uint8_t* cmd = _path.commandData();

    size_t i = 0;
    while (i < count) {
      switch (cmd[i]) {
        case BL_PATH_CMD_MOVE:
          printf("p.moveTo(%g, %g);\n", vtx[i].x, vtx[i].y);
          i++;
          break;
        case BL_PATH_CMD_ON:
          printf("p.lineTo(%g, %g);\n", vtx[i].x, vtx[i].y);
          i++;
          break;
        case BL_PATH_CMD_QUAD:
          printf("p.quadTo(%g, %g, %g, %g);\n", vtx[i].x, vtx[i].y, vtx[i+1].x, vtx[i+1].y);
          i += 2;
          break;
        case BL_PATH_CMD_CUBIC:
          printf("p.cubicTo(%g, %g, %g, %g, %g, %g);\n", vtx[i].x, vtx[i].y, vtx[i+1].x, vtx[i+1].y, vtx[i+2].x, vtx[i+2].y);
          i += 3;
          break;
        case BL_PATH_CMD_CLOSE:
          printf("p.close();\n");
          i++;
          break;
      }
    }
  }

  Q_SLOT void onCapTypeUpdate(int index) {
    _strokeOptions.startCap = uint8_t(_capTypeSelect.itemData(index).toInt());
    _strokeOptions.endCap = _strokeOptions.startCap;
    _canvas.updateCanvas();
  }

  Q_SLOT void onJoinTypeUpdate(int index) {
    _strokeOptions.join = uint8_t(_joinTypeSelect.itemData(index).toInt());
    _canvas.updateCanvas();
  }

  Q_SLOT void onWidthChanged(int value) {
    _strokeOptions.width = double(value);
    _canvas.updateCanvas();
  }

  Q_SLOT void onMiterLimitChanged(int value) {
    _strokeOptions.miterLimit = double(value) / 100.0;
    _canvas.updateCanvas();
  }

  void onRender(BLContext& ctx) {
    ctx.setFillStyle(BLRgba32(0xFF000000u));
    ctx.fillAll();

    BLPath s;
    s.addStrokedPath(_path, _strokeOptions, blDefaultApproximationOptions);

    ctx.setFillStyle(BLRgba32(0x8F003FAAu));
    ctx.fillPath(s);

    if (_showControl) {
      ctx.setStrokeStyle(BLRgba32(0xFF0066AAu));
      ctx.strokePath(s);
      renderPathPoints(ctx, s, SIZE_MAX, BLRgba32(0x7F007FFFu), BLRgba32(0xFFFFFFFFu));
    }

    ctx.setStrokeStyle(BLRgba32(0xFFFFFFFFu));
    ctx.strokePath(_path);

    renderPathPoints(ctx, _path, _closestVertex, BLRgba32(0xFFFFFFFF), BLRgba32(0xFF00FFFFu));
  }

  void renderPathPoints(BLContext& ctx, const BLPath& path, size_t highlight, BLRgba32 normalColor, BLRgba32 highlightColor) noexcept {
    size_t count = path.size();
    const BLPoint* vtx = path.vertexData();

    for (size_t i = 0; i < count; i++) {
      if (!std::isfinite(vtx[i].x))
        continue;
      if (i == highlight)
        ctx.setFillStyle(highlightColor);
      else
        ctx.setFillStyle(normalColor);

      ctx.fillCircle(vtx[i].x, vtx[i].y, 2.5);
    }
  }
};

int main(int argc, char *argv[]) {
  QApplication app(argc, argv);
  MainWindow win;

  win.resize(QSize(580, 520));
  win.show();

  return app.exec();
}

#include "bl_qt_stroke.moc"

#include "qblcanvas.h"
#include "bl-qt-tiger.h"

#include <stdlib.h>

#define ARRAY_SIZE(X) uint32_t(sizeof(X) / sizeof(X[0]))

static const double PI = 3.14159265359;

struct TigerPath {
  inline TigerPath()
    : fillColor(0),
      strokeColor(0),
      qtPen(Qt::NoPen),
      fillRule(BL_FILL_RULE_NON_ZERO),
      fill(false),
      stroke(false) {}
  inline ~TigerPath() {}

  BLPath blPath;
  BLPath blStrokedPath;
  BLStrokeOptions blStrokeOptions;

  BLRgba32 fillColor;
  BLRgba32 strokeColor;

  QPainterPath qtPath;
  QPainterPath qtStrokedPath;
  QPen qtPen;
  QBrush qtBrush;

  uint32_t fillRule;
  bool fill;
  bool stroke;
};

struct Tiger {
  Tiger() {
    init(
      TigerData::commands, ARRAY_SIZE(TigerData::commands),
      TigerData::points, ARRAY_SIZE(TigerData::points));
  }

  ~Tiger() {
    destroy();
  }

  void init(const char* commands, int commandCount, const float* points, int pointCount) {
    size_t c = 0;
    size_t p = 0;

    float h = TigerData::height;

    while (c < commandCount) {
      TigerPath* tp = new TigerPath();

      // Fill params.
      switch (commands[c++]) {
        case 'N': tp->fill = false; break;
        case 'F': tp->fill = true; tp->fillRule = BL_FILL_RULE_NON_ZERO; break;
        case 'E': tp->fill = true; tp->fillRule = BL_FILL_RULE_EVEN_ODD; break;
      }

      // Stroke params.
      switch (commands[c++]) {
        case 'N': tp->stroke = false; break;
        case 'S': tp->stroke = true; break;
      }

      switch (commands[c++]) {
        case 'B': tp->blStrokeOptions.setCaps(BL_STROKE_CAP_BUTT); break;
        case 'R': tp->blStrokeOptions.setCaps(BL_STROKE_CAP_ROUND); break;
        case 'S': tp->blStrokeOptions.setCaps(BL_STROKE_CAP_SQUARE); break;
      }

      switch (commands[c++]) {
        case 'M': tp->blStrokeOptions.join = BL_STROKE_JOIN_MITER_BEVEL; break;
        case 'R': tp->blStrokeOptions.join = BL_STROKE_JOIN_ROUND; break;
        case 'B': tp->blStrokeOptions.join = BL_STROKE_JOIN_BEVEL; break;
      }

      tp->blStrokeOptions.miterLimit = points[p++];
      tp->blStrokeOptions.width = points[p++];

      // Stroke & Fill style.
      tp->strokeColor = BLRgba32(uint32_t(points[p + 0] * 255.0f), uint32_t(points[p + 1] * 255.0f), uint32_t(points[p + 2] * 255.0f), 255);
      tp->fillColor = BLRgba32(uint32_t(points[p + 3] * 255.0f), uint32_t(points[p + 4] * 255.0f), uint32_t(points[p + 5] * 255.0f), 255);
      p += 6;

      // Path.
      int i, count = int(points[p++]);
      for (i = 0 ; i < count; i++) {
        switch (commands[c++]) {
          case 'M':
            tp->blPath.moveTo(points[p], h - points[p + 1]);
            tp->qtPath.moveTo(points[p], h - points[p + 1]);
            p += 2;
            break;
          case 'L':
            tp->blPath.lineTo(points[p], h - points[p + 1]);
            tp->qtPath.lineTo(points[p], h - points[p + 1]);
            p += 2;
            break;
          case 'C':
            tp->blPath.cubicTo(points[p], h - points[p + 1], points[p + 2], h - points[p + 3], points[p + 4], h - points[p + 5]);
            tp->qtPath.cubicTo(points[p], h - points[p + 1], points[p + 2], h - points[p + 3], points[p + 4], h - points[p + 5]);
            p += 6;
            break;
          case 'E':
            tp->blPath.close();
            tp->qtPath.closeSubpath();
            break;
        }
      }

      tp->blPath.shrink();
      tp->qtPath.setFillRule(tp->fillRule == BL_FILL_RULE_NON_ZERO ? Qt::WindingFill : Qt::OddEvenFill);

      if (tp->fill) {
        tp->qtBrush = QBrush(QColor(qRgb(tp->fillColor.r, tp->fillColor.g, tp->fillColor.b)));
      }

      if (tp->stroke) {
        tp->blStrokedPath.addStrokedPath(tp->blPath, tp->blStrokeOptions, blDefaultApproximationOptions);
        tp->blStrokedPath.shrink();

        tp->qtPen = QPen(QColor(qRgb(tp->strokeColor.r, tp->strokeColor.g, tp->strokeColor.b)));
        tp->qtPen.setWidthF(tp->blStrokeOptions.width);
        tp->qtPen.setMiterLimit(tp->blStrokeOptions.miterLimit);

        Qt::PenCapStyle qtCapStyle =
          tp->blStrokeOptions.startCap == BL_STROKE_CAP_BUTT  ? Qt::FlatCap  :
          tp->blStrokeOptions.startCap == BL_STROKE_CAP_ROUND ? Qt::RoundCap : Qt::SquareCap;

        Qt::PenJoinStyle qtJoinStyle =
          tp->blStrokeOptions.join == BL_STROKE_JOIN_ROUND ? Qt::RoundJoin :
          tp->blStrokeOptions.join == BL_STROKE_JOIN_BEVEL ? Qt::BevelJoin : Qt::MiterJoin;

        tp->qtPen.setCapStyle(qtCapStyle);
        tp->qtPen.setJoinStyle(qtJoinStyle);

        QPainterPathStroker stroker;
        stroker.setWidth(tp->blStrokeOptions.width);
        stroker.setMiterLimit(tp->blStrokeOptions.miterLimit);
        stroker.setJoinStyle(qtJoinStyle);
        stroker.setCapStyle(qtCapStyle);

        tp->qtStrokedPath = stroker.createStroke(tp->qtPath);
      }

      paths.append(tp);
    }
  }

  void destroy() {
    for (size_t i = 0, count = paths.size(); i < count; i++)
      delete paths[i];
    paths.reset();
  }

  BLArray<TigerPath*> paths;
};

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
  QComboBox _cachingSelect;
  QSlider _slider;
  Tiger _tiger;
  double _rot;
  double _scale;
  bool _rotateEachFrame;
  bool _cacheStroke;

  MainWindow() {
    QVBoxLayout* vBox = new QVBoxLayout();
    vBox->setContentsMargins(0, 0, 0, 0);
    vBox->setSpacing(0);

    QGridLayout* grid = new QGridLayout();
    grid->setContentsMargins(5, 5, 5, 5);
    grid->setSpacing(5);

    _rendererSelect.addItem("Blend2D", QVariant(int(QBLCanvas::RendererB2D)));
    _rendererSelect.addItem("Qt", QVariant(int(QBLCanvas::RendererQt)));
    _limitFpsCheck.setText(QLatin1Literal("Limit FPS"));

    _cachingSelect.addItem("None", QVariant(int(0)));
    _cachingSelect.addItem("Strokes", QVariant(int(1)));

    _slider.setOrientation(Qt::Horizontal);
    _slider.setMinimum(50);
    _slider.setMaximum(20000);
    _slider.setSliderPosition(1000);

    connect(&_rendererSelect, SIGNAL(currentIndexChanged(int)), SLOT(onRendererChanged(int)));
    connect(&_limitFpsCheck, SIGNAL(stateChanged(int)), SLOT(onLimitFpsChanged(int)));
    connect(&_cachingSelect, SIGNAL(currentIndexChanged(int)), SLOT(onCachingChanged(int)));
    connect(&_slider, SIGNAL(valueChanged(int)), SLOT(onZoomChanged(int)));

    _canvas.onRenderB2D = std::bind(&MainWindow::onRenderB2D, this, std::placeholders::_1);
    _canvas.onRenderQt = std::bind(&MainWindow::onRenderQt, this, std::placeholders::_1);

    grid->addWidget(new QLabel("Renderer:"), 0, 0, Qt::AlignRight);
    grid->addWidget(&_rendererSelect, 0, 1);
    grid->addWidget(&_limitFpsCheck, 0, 2);

    grid->addWidget(new QLabel("Caching:"), 1, 0, Qt::AlignRight);
    grid->addWidget(&_cachingSelect, 1, 1);
    grid->addWidget(&_slider, 1, 2);

    _canvas.onRenderB2D = std::bind(&MainWindow::onRenderB2D, this, std::placeholders::_1);
    _canvas.onRenderQt = std::bind(&MainWindow::onRenderQt, this, std::placeholders::_1);

    vBox->addLayout(grid);
    vBox->addWidget(&_canvas);
    setLayout(vBox);

    connect(&_timer, SIGNAL(timeout()), this, SLOT(onTimer()));
    _timer.setInterval(2);

    connect(new QShortcut(QKeySequence(Qt::Key_R), this), SIGNAL(activated()), SLOT(onToggleRenderer()));
    connect(new QShortcut(QKeySequence(Qt::Key_B), this), SIGNAL(activated()), SLOT(onToggleRotate()));
    connect(new QShortcut(QKeySequence(Qt::Key_Q), this), SIGNAL(activated()), SLOT(onRotatePrev()));
    connect(new QShortcut(QKeySequence(Qt::Key_W), this), SIGNAL(activated()), SLOT(onRotateNext()));

    onInit();
    _updateTitle();
  }

  void showEvent(QShowEvent* event) override { _timer.start(); }
  void hideEvent(QHideEvent* event) override { _timer.stop(); }

  void onInit() {
    _rot = 0.0;
    _scale = 1.0;
    _rotateEachFrame = true;
    _cacheStroke = false;
  }

  Q_SLOT void onRendererChanged(int index) { _canvas.setRendererType(_rendererSelect.itemData(index).toInt()); }
  Q_SLOT void onLimitFpsChanged(int value) { _timer.setInterval(value ? 1000 / 60 : 2); }

  Q_SLOT void onCachingChanged(int index) { _cacheStroke = index != 0; }
  Q_SLOT void onZoomChanged(int value) { _scale = (double(value) / 1000.0); }

  Q_SLOT void onToggleRenderer() { _rendererSelect.setCurrentIndex(_rendererSelect.currentIndex() ^ 1); }
  Q_SLOT void onToggleRotate() { _rotateEachFrame = !_rotateEachFrame; }
  Q_SLOT void onRotatePrev() { _rot -= 0.25; }
  Q_SLOT void onRotateNext() { _rot += 0.25; }

  Q_SLOT void onTimer() {
    if (_rotateEachFrame) {
      _rot += 0.25;
      if (_rot >= 360) _rot -= 360.0;
    }

    _canvas.updateCanvas(true);
    _updateTitle();
  }

  void onRenderB2D(BLContext& ctx) noexcept {
    ctx.setFillStyle(BLRgba32(0xFF00007F));
    ctx.fillAll();

    double minX = 17;
    double minY = 53;
    double maxX = 562.0;
    double maxY = 613.0;
    double s = blMin(_canvas.width() / (maxX - minX), _canvas.height() / (maxY - minY)) * _scale;

    BLMatrix2D m;
    m.reset();
    m.rotate((_rot / 180.0) * PI, minX + maxX / 2.0, minY + maxY / 2.0);
    m.postTranslate(-maxX / 2, -maxY / 2);

    ctx.save();
    ctx.translate(_canvas.width() / 2, _canvas.height() / 2);
    ctx.scale(s);
    ctx.transform(m);

    for (size_t i = 0, count = _tiger.paths.size(); i < count; i++) {
      const TigerPath* tp = _tiger.paths[i];

      if (tp->fill) {
        ctx.setFillStyle(tp->fillColor);
        ctx.setFillRule(tp->fillRule);
        ctx.fillPath(tp->blPath);
      }

      if (tp->stroke) {
        if (_cacheStroke) {
          ctx.setFillStyle(tp->strokeColor);
          ctx.fillPath(tp->blStrokedPath);
        }
        else {
          ctx.setStrokeStyle(tp->strokeColor);
          ctx.setStrokeOptions(tp->blStrokeOptions);
          ctx.strokePath(tp->blPath);
        }
      }
    }

    ctx.restore();
  }

  void onRenderQt(QPainter& ctx) noexcept {
    ctx.fillRect(0, 0, _canvas.width(), _canvas.height(), QColor(0, 0, 0x7F));
    ctx.setRenderHint(QPainter::Antialiasing, true);

    double minX = 17;
    double minY = 53;
    double maxX = 562.0;
    double maxY = 613.0;
    double s = blMin(_canvas.width() / (maxX - minX), _canvas.height() / (maxY - minY)) * _scale;

    BLMatrix2D m;
    m.reset();
    m.rotate((_rot / 180.0) * PI, minX + maxX / 2.0, minY + maxY / 2.0);
    m.postTranslate(-maxX / 2, -maxY / 2);

    ctx.save();
    ctx.translate(_canvas.width() / 2, _canvas.height() / 2);
    ctx.scale(s, s);
    ctx.setMatrix(QMatrix(m.m00, m.m01, m.m10, m.m11, m.m20, m.m21), true);

    for (size_t i = 0, count = _tiger.paths.size(); i < count; i++) {
      const TigerPath* tp = _tiger.paths[i];

      if (tp->fill) {
        ctx.fillPath(tp->qtPath, tp->qtBrush);
      }

      if (tp->stroke) {
        if (_cacheStroke)
          ctx.fillPath(tp->qtStrokedPath, tp->qtPen.brush());
        else
          ctx.strokePath(tp->qtPath, tp->qtPen);
      }
    }

    ctx.restore();
  }

  void _updateTitle() {
    char buf[256];
    snprintf(buf, 256, "Tiger Sample [%dx%d] [%.1f FPS]",
      _canvas.width(),
      _canvas.height(),
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

#include "bl-qt-tiger.moc"

#include <stdlib.h>
#include <vector>

#include "bl_qt_headers.h"
#include "bl_qt_canvas.h"

#include "../bl_bench/images_data.h"

class MainWindow : public QWidget {
  Q_OBJECT

public:
  QTimer _timer;
  QSlider _countSlider;
  QComboBox _rendererSelect;
  QComboBox _compOpSelect;
  QCheckBox _limitFpsCheck;
  QBLCanvas _canvas;

  BLRandom _random;
  std::vector<BLPoint> _coords;
  std::vector<BLPoint> _steps;
  std::vector<uint32_t> _spriteIds;

  bool _animate = true;
  BLCompOp _compOp = BL_COMP_OP_SRC_OVER;

  BLImage _spritesB2D[4];
  QImage _spritesQt[4];

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
    _compOpSelect.addItem("SrcAtop", QVariant(int(BL_COMP_OP_SRC_ATOP)));
    _compOpSelect.addItem("DstAtop", QVariant(int(BL_COMP_OP_DST_ATOP)));
    _compOpSelect.addItem("Xor", QVariant(int(BL_COMP_OP_XOR)));
    _compOpSelect.addItem("Plus", QVariant(int(BL_COMP_OP_PLUS)));
    _compOpSelect.addItem("Multiply", QVariant(int(BL_COMP_OP_MULTIPLY)));
    _compOpSelect.addItem("Screen", QVariant(int(BL_COMP_OP_SCREEN)));
    _compOpSelect.addItem("Overlay", QVariant(int(BL_COMP_OP_OVERLAY)));
    _compOpSelect.addItem("Darken", QVariant(int(BL_COMP_OP_DARKEN)));
    _compOpSelect.addItem("Lighten", QVariant(int(BL_COMP_OP_LIGHTEN)));
    _compOpSelect.addItem("Color Dodge", QVariant(int(BL_COMP_OP_COLOR_DODGE)));
    _compOpSelect.addItem("Color Burn", QVariant(int(BL_COMP_OP_COLOR_BURN)));
    _compOpSelect.addItem("Hard Light", QVariant(int(BL_COMP_OP_HARD_LIGHT)));
    _compOpSelect.addItem("Soft Light", QVariant(int(BL_COMP_OP_SOFT_LIGHT)));
    _compOpSelect.addItem("Difference", QVariant(int(BL_COMP_OP_DIFFERENCE)));
    _compOpSelect.addItem("Exclusion", QVariant(int(BL_COMP_OP_EXCLUSION)));

    _limitFpsCheck.setText(QLatin1String("Limit FPS"));

    _countSlider.setOrientation(Qt::Horizontal);
    _countSlider.setMinimum(1);
    _countSlider.setMaximum(10000);
    _countSlider.setSliderPosition(200);

    _canvas.onRenderB2D = std::bind(&MainWindow::onRenderB2D, this, std::placeholders::_1);
    _canvas.onRenderQt = std::bind(&MainWindow::onRenderQt, this, std::placeholders::_1);

    connect(&_rendererSelect, SIGNAL(activated(int)), SLOT(onRendererChanged(int)));
    connect(&_compOpSelect, SIGNAL(activated(int)), SLOT(onCompOpChanged(int)));
    connect(&_limitFpsCheck, SIGNAL(stateChanged(int)), SLOT(onLimitFpsChanged(int)));
    connect(&_countSlider, SIGNAL(valueChanged(int)), SLOT(onCountChanged(int)));

    grid->addWidget(new QLabel("Renderer:"), 0, 0);
    grid->addWidget(&_rendererSelect, 0, 1);

    grid->addWidget(new QLabel("Comp Op:"), 0, 2);
    grid->addWidget(&_compOpSelect, 0, 3);

    grid->addItem(new QSpacerItem(0, 0, QSizePolicy::Expanding), 0, 4);
    grid->addWidget(&_limitFpsCheck, 0, 5, Qt::AlignRight);

    grid->addWidget(new QLabel("Count:"), 1, 0, 1, 1, Qt::AlignRight);
    grid->addWidget(&_countSlider, 1, 1, 1, 7);

    vBox->addLayout(grid);
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
    _spritesB2D[0].readFromData(_resource_babelfish_png, sizeof(_resource_babelfish_png));
    _spritesB2D[1].readFromData(_resource_ksplash_png  , sizeof(_resource_ksplash_png  ));
    _spritesB2D[2].readFromData(_resource_ktip_png     , sizeof(_resource_ktip_png     ));
    _spritesB2D[3].readFromData(_resource_firewall_png , sizeof(_resource_firewall_png ));

    for (uint32_t i = 0; i < 4; i++) {
      const BLImage& sprite = _spritesB2D[i];

      BLImageData spriteData;
      sprite.getData(&spriteData);

      _spritesQt[i] = QImage(
        static_cast<unsigned char*>(spriteData.pixelData),
        spriteData.size.w,
        spriteData.size.h,
        int(spriteData.stride), QImage::Format_ARGB32_Premultiplied);
    }

    setCount(_countSlider.sliderPosition());
    _limitFpsCheck.setChecked(true);
    _updateTitle();
  }

  double randomSign() noexcept { return _random.nextDouble() < 0.5 ? 1.0 : -1.0; }

  Q_SLOT void onToggleAnimate() { _animate = !_animate; }
  Q_SLOT void onRendererChanged(int index) { _canvas.setRendererType(_rendererSelect.itemData(index).toInt());  }
  Q_SLOT void onCompOpChanged(int index) { _compOp = (BLCompOp)_compOpSelect.itemData(index).toInt(); };
  Q_SLOT void onLimitFpsChanged(int value) { _timer.setInterval(value ? 1000 / 120 : 0); }
  Q_SLOT void onCountChanged(int value) { setCount(size_t(value)); }

  Q_SLOT void onTimer() {
    if (_animate) {
      double w = _canvas.blImage.width();
      double h = _canvas.blImage.height();

      size_t size = _coords.size();
      for (size_t i = 0; i < size; i++) {
        BLPoint& vertex = _coords[i];
        BLPoint& step = _steps[i];

        vertex += step;
        if (vertex.x < 0 || vertex.x >= w) {
          vertex.x -= step.x;
          if (vertex.x < 0) vertex.x = 0;
          if (vertex.x >= w) vertex.x = w - 1;
          step.x = -step.x;
        }

        if (vertex.y < 0 || vertex.y >= h) {
          vertex.y -= step.y;
          if (vertex.y < 0) vertex.y = 0;
          if (vertex.y >= h) vertex.y = h - 1;
          step.y = -step.y;
        }
      }
    }

    _canvas.updateCanvas(true);
    _updateTitle();
  }

  void onRenderB2D(BLContext& ctx) noexcept {
    ctx.setCompOp(BL_COMP_OP_SRC_COPY);
    ctx.fillAll(blBackgroundForCompOp(_compOp));
    ctx.setCompOp(_compOp);

    size_t size = _coords.size();
    int rectSize = 128;
    int halfSize = rectSize / 2;

    for (size_t i = 0; i < size; i++) {
      int x = int(_coords[i].x) - halfSize;
      int y = int(_coords[i].y) - halfSize;
      ctx.blitImage(BLPointI(x, y), _spritesB2D[_spriteIds[i]]);
    }
  }

  void onRenderQt(QPainter& ctx) noexcept {
    ctx.setCompositionMode(QPainter::CompositionMode_Source);
    ctx.fillRect(0, 0, _canvas.width(), _canvas.height(), blRgbaToQColor(blBackgroundForCompOp(_compOp)));
    ctx.setRenderHint(QPainter::Antialiasing, true);
    ctx.setCompositionMode(blCompOpToQPainterCompositionMode(_compOp));

    size_t size = _coords.size();
    int rectSize = 128;
    int halfSize = rectSize / 2;

    for (size_t i = 0; i < size; i++) {
      int x = int(_coords[i].x) - halfSize;
      int y = int(_coords[i].y) - halfSize;
      ctx.drawImage(QPoint(x, y), _spritesQt[_spriteIds[i]]);
    }
  }

  void setCount(size_t size) {
    int w = _canvas.width();
    int h = _canvas.height();
    size_t i = _coords.size();

    if (w < 16) w = 128;
    if (h < 16) h = 128;

    _coords.resize(size);
    _steps.resize(size);
    _spriteIds.resize(size);

    while (i < size) {
      _coords[i].reset(_random.nextDouble() * (w - 1),
                       _random.nextDouble() * (h - 1));
      _steps[i].reset((_random.nextDouble() * 2 + 1) * randomSign(),
                      (_random.nextDouble() * 2 + 1) * randomSign());
      _spriteIds[i] = _random.nextUInt32() % 4u;
      i++;
    }
  }

  void _updateTitle() {
    char buf[256];
    snprintf(buf, 256, "Sprites [%dx%d] [Count=%zu] [RenderTime=%.2fms FPS=%.1f]",
      _canvas.width(),
      _canvas.height(),
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

#include "bl_sprites_demo.moc"

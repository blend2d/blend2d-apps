#include <stdlib.h>
#include <cmath>
#include <vector>

#include "bl_qt_headers.h"
#include "bl_qt_canvas.h"

#include "../bl_bench/images_data.h"

class MainWindow : public QWidget {
  Q_OBJECT

public:
  QTimer _timer;
  QComboBox _rendererSelect;
  QComboBox _extendModeSelect;
  QCheckBox _limitFpsCheck;
  QCheckBox _bilinearCheck;
  QCheckBox _fillPathCheck;
  QSlider _fracX;
  QSlider _fracY;
  QSlider _angle;
  QSlider _scale;
  QBLCanvas _canvas;
  BLImage _spritesB2D[4];
  QImage _spritesQt[4];

  MainWindow() {
    QVBoxLayout* vBox = new QVBoxLayout();
    vBox->setContentsMargins(0, 0, 0, 0);
    vBox->setSpacing(0);

    QGridLayout* grid = new QGridLayout();
    grid->setContentsMargins(5, 5, 5, 5);
    grid->setSpacing(5);

    QBLCanvas::initRendererSelectBox(&_rendererSelect);
    _limitFpsCheck.setText(QLatin1String("Limit FPS"));
    _bilinearCheck.setText(QLatin1String("Bilinear"));
    _fillPathCheck.setText(QLatin1String("Fill Path"));

    static const char* extendModeNames[] = {
      "PAD",
      "REPEAT",
      "REFLECT",
      "PAD-X PAD-Y",
      "PAD-X REPEAT-Y",
      "PAD-X REFLECT-Y",
      "REPEAT-X PAD-Y",
      "REPEAT-X REPEAT-Y",
      "REPEAT-X REFLECT-Y",
      "REFLECT-X PAD-Y",
      "REFLECT-X REPEAT-Y",
      "REFLECT-X REFLECT-Y"
    };

    static const BLExtendMode extendModeValues[] = {
      BL_EXTEND_MODE_PAD,
      BL_EXTEND_MODE_REPEAT,
      BL_EXTEND_MODE_REFLECT,
      BL_EXTEND_MODE_PAD_X_PAD_Y,
      BL_EXTEND_MODE_PAD_X_REPEAT_Y,
      BL_EXTEND_MODE_PAD_X_REFLECT_Y,
      BL_EXTEND_MODE_REPEAT_X_PAD_Y,
      BL_EXTEND_MODE_REPEAT_X_REPEAT_Y,
      BL_EXTEND_MODE_REPEAT_X_REFLECT_Y,
      BL_EXTEND_MODE_REFLECT_X_PAD_Y,
      BL_EXTEND_MODE_REFLECT_X_REPEAT_Y,
      BL_EXTEND_MODE_REFLECT_X_REFLECT_Y
    };

    for (uint32_t i = 0; i < 12; i++) {
      QString s = extendModeNames[i];
      _extendModeSelect.addItem(s, QVariant(int(extendModeValues[i])));
    }
    _extendModeSelect.setCurrentIndex(1);

    _fracX.setMinimum(0);
    _fracX.setMaximum(255);
    _fracX.setValue(0);
    _fracX.setOrientation(Qt::Horizontal);

    _fracY.setMinimum(0);
    _fracY.setMaximum(255);
    _fracY.setValue(0);
    _fracY.setOrientation(Qt::Horizontal);

    _angle.setMinimum(0);
    _angle.setMaximum(3600);
    _angle.setValue(0);
    _angle.setOrientation(Qt::Horizontal);

    _scale.setMinimum(0);
    _scale.setMaximum(1000);
    _scale.setValue(0);
    _scale.setOrientation(Qt::Horizontal);

    connect(&_rendererSelect, SIGNAL(activated(int)), SLOT(onRendererChanged(int)));
    connect(&_limitFpsCheck, SIGNAL(stateChanged(int)), SLOT(onLimitFpsChanged(int)));
    connect(&_bilinearCheck, SIGNAL(valueChanged(int)), SLOT(onSliderChanged(int)));
    connect(&_fillPathCheck, SIGNAL(valueChanged(int)), SLOT(onSliderChanged(int)));
    connect(&_extendModeSelect, SIGNAL(activated(int)), SLOT(onSliderChanged(int)));
    connect(&_fracX, SIGNAL(valueChanged(int)), SLOT(onSliderChanged(int)));
    connect(&_fracY, SIGNAL(valueChanged(int)), SLOT(onSliderChanged(int)));
    connect(&_angle, SIGNAL(valueChanged(int)), SLOT(onSliderChanged(int)));
    connect(&_scale, SIGNAL(valueChanged(int)), SLOT(onSliderChanged(int)));

    grid->addWidget(new QLabel("Renderer:"), 0, 0);
    grid->addWidget(&_rendererSelect, 0, 1);

    grid->addItem(new QSpacerItem(0, 0, QSizePolicy::Expanding), 0, 4);
    grid->addWidget(&_limitFpsCheck, 0, 5);

    grid->addWidget(new QLabel("Extend Mode:"), 1, 0);
    grid->addWidget(&_extendModeSelect, 1, 1);

    grid->addWidget(new QLabel("Fx Offset:"), 0, 2);
    grid->addWidget(&_fracX, 0, 3, 1, 2);

    grid->addItem(new QSpacerItem(1, 0, QSizePolicy::Expanding), 0, 4);
    grid->addWidget(&_bilinearCheck, 1, 5);

    grid->addItem(new QSpacerItem(2, 0, QSizePolicy::Expanding), 0, 4);
    grid->addWidget(&_fillPathCheck, 2, 5);

    grid->addWidget(new QLabel("Fy Offset:"), 1, 2);
    grid->addWidget(&_fracY, 1, 3, 1, 2);

    grid->addWidget(new QLabel("Angle:"), 2, 0);
    grid->addWidget(&_angle, 2, 1, 1, 4);

    grid->addWidget(new QLabel("Scale:"), 3, 0);
    grid->addWidget(&_scale, 3, 1, 1, 4);

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
    _limitFpsCheck.setChecked(true);
    _bilinearCheck.setChecked(true);

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

    _updateTitle();
  }

  Q_SLOT void onRendererChanged(int index) { _canvas.setRendererType(_rendererSelect.itemData(index).toInt()); }
  Q_SLOT void onLimitFpsChanged(int value) { _timer.setInterval(value ? 1000 / 120 : 0); }

  Q_SLOT void onSliderChanged(int value) { _canvas.updateCanvas(true); }

  Q_SLOT void onTimer() {
    _canvas.updateCanvas(true);
    _updateTitle();
  }

  inline double tx() const { return 256.0 + double(_fracX.value()) / 256.0; }
  inline double ty() const { return 256.0 + double(_fracY.value()) / 256.0; }
  inline double angleRad() const { return (double(_angle.value()) / (3600.0 / 2)) * M_PI; }
  inline double scale() const { return double(_scale.value() + 100) / 100.0; }

  void onRenderB2D(BLContext& ctx) noexcept {
    int rx = _canvas.width() / 2;
    int ry = _canvas.height() / 2;

    BLPattern pattern(_spritesB2D[0], BLExtendMode(_extendModeSelect.currentData().toInt()));
    pattern.rotate(angleRad(), rx, ry);
    pattern.translate(tx(), ty());
    pattern.scale(scale());

    ctx.setPatternQuality(
      _bilinearCheck.isChecked()
        ? BL_PATTERN_QUALITY_BILINEAR
        : BL_PATTERN_QUALITY_NEAREST);

    ctx.setCompOp(BL_COMP_OP_SRC_COPY);

    if (_fillPathCheck.isChecked()) {
      ctx.clearAll();
      ctx.fillCircle(rx, ry, blMin(rx, ry), pattern);
    }
    else {
      ctx.fillAll(pattern);
    }
  }

  void onRenderQt(QPainter& ctx) noexcept {
    int rx = _canvas.width() / 2;
    int ry = _canvas.height() / 2;

    QTransform tr;
    tr.translate(rx, ry);
    tr.rotateRadians(angleRad());
    tr.translate(-rx + tx(), -ry + ty());
    tr.scale(scale(), scale());

    QBrush brush(_spritesQt[0]);
    brush.setTransform(tr);

    ctx.setRenderHint(QPainter::SmoothPixmapTransform, _bilinearCheck.isChecked());
    ctx.setRenderHint(QPainter::Antialiasing, true);
    ctx.setCompositionMode(QPainter::CompositionMode_Source);

    if (_fillPathCheck.isChecked()) {
      double r = blMin(rx, ry);
      ctx.fillRect(QRect(0, 0, _canvas.width(), _canvas.height()), QColor(0, 0, 0, 0));
      ctx.setBrush(brush);
      ctx.setPen(Qt::NoPen);
      ctx.drawEllipse(QPointF(qreal(rx), qreal(ry)), qreal(r), qreal(r));
    }
    else {
      ctx.fillRect(QRect(0, 0, _canvas.width(), _canvas.height()), brush);
    }
  }

  void _updateTitle() {
    char buf[256];

    snprintf(buf, 256, "Patterns [%dx%d] [RenderTime=%.2fms FPS=%.1f]",
      _canvas.width(),
      _canvas.height(),
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

  win.setMinimumSize(QSize(20 + (128 + 10) * 4 + 20, 20 + (128 + 10) * 4 + 20));
  win.resize(QSize(580, 520));
  win.show();

  return app.exec();
}

#include "bl_pattern_demo.moc"

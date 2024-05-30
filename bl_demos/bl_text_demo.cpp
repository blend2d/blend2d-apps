#include <blend2d.h>
#include <chrono>

#include "bl_qt_headers.h"
#include "bl_qt_canvas.h"

class PerformanceTimer {
public:
  typedef std::chrono::high_resolution_clock::time_point TimePoint;

  TimePoint _startTime {};
  TimePoint _endTime {};

  inline void start() {
    _startTime = std::chrono::high_resolution_clock::now();
  }

  inline void stop() {
    _endTime = std::chrono::high_resolution_clock::now();
  }

  inline double duration() const {
    std::chrono::duration<double> elapsed = _endTime - _startTime;
    return elapsed.count() * 1000;
  }
};

static void debugGlyphBufferSink(const char* message, size_t size, void* userData) noexcept {
  BLString* buffer = static_cast<BLString*>(userData);
  buffer->append(message, size);
  buffer->append('\n');
}

class MainWindow : public QWidget {
  Q_OBJECT

public:
  // Widgets.
  QComboBox* _rendererSelect {};
  QComboBox* _styleSelect {};
  QLineEdit* _fileSelected {};
  QPushButton* _fileSelectButton {};
  QSlider* _slider {};
  QLineEdit* _text {};
  QBLCanvas* _canvas {};
  QCheckBox* _otDebug {};

  int _qtApplicationFontId = -1;

  // Loaded font.
  BLFontFace _blFace;
  QFont _qtFont;
  QRawFont _qtRawFont;

  MainWindow() {
    QVBoxLayout* vBox = new QVBoxLayout();
    vBox->setContentsMargins(0, 0, 0, 0);
    vBox->setSpacing(0);

    QGridLayout* grid = new QGridLayout();
    grid->setContentsMargins(5, 5, 5, 5);
    grid->setSpacing(5);

    _rendererSelect = new QComboBox();
    QBLCanvas::initRendererSelectBox(_rendererSelect);

    _styleSelect = new QComboBox();
    _styleSelect->addItem("Solid Color", QVariant(int(0)));
    _styleSelect->addItem("Linear Gradient", QVariant(int(1)));
    _styleSelect->addItem("Radial Gradient", QVariant(int(2)));
    _styleSelect->addItem("Conic Gradient", QVariant(int(3)));

    _fileSelected = new QLineEdit("");
    _fileSelectButton = new QPushButton("Select...");
    _slider = new QSlider();
    _canvas = new QBLCanvas();

    _slider->setOrientation(Qt::Horizontal);
    _slider->setMinimum(5);
    _slider->setMaximum(400);
    _slider->setSliderPosition(20);

    _text = new QLineEdit();
    _text->setText(QString("Test"));

    _otDebug = new QCheckBox();
    _otDebug->setText(QLatin1String("OpenType Dbg"));

    connect(_rendererSelect, SIGNAL(activated(int)), SLOT(onRendererChanged(int)));
    connect(_styleSelect, SIGNAL(activated(int)), SLOT(onStyleChanged(int)));
    connect(_otDebug, SIGNAL(stateChanged(int)), SLOT(valueChanged(int)));
    connect(_fileSelectButton, SIGNAL(clicked()), SLOT(selectFile()));
    connect(_fileSelected, SIGNAL(textChanged(const QString&)), SLOT(fileChanged(const QString&)));
    connect(_slider, SIGNAL(valueChanged(int)), SLOT(valueChanged(int)));
    connect(_text, SIGNAL(textChanged(const QString&)), SLOT(textChanged(const QString&)));

    _canvas->onRenderB2D = std::bind(&MainWindow::onRenderB2D, this, std::placeholders::_1);
    _canvas->onRenderQt = std::bind(&MainWindow::onRenderQt, this, std::placeholders::_1);

    grid->addWidget(new QLabel("Renderer:"), 0, 0);
    grid->addWidget(_rendererSelect, 0, 1);
    grid->addWidget(_otDebug, 0, 4);

    grid->addWidget(new QLabel("Style:"), 1, 0);
    grid->addWidget(_styleSelect, 1, 1);

    grid->addWidget(new QLabel("Font:"), 2, 0);
    grid->addWidget(_fileSelected, 2, 1, 1, 3);
    grid->addWidget(_fileSelectButton, 2, 4);

    grid->addWidget(new QLabel("Size:"), 3, 0);
    grid->addWidget(_slider, 3, 1, 1, 4);

    grid->addWidget(new QLabel("Text:"), 4, 0);
    grid->addWidget(_text, 4, 1, 1, 4);

    vBox->addItem(grid);
    vBox->addWidget(_canvas);

    setLayout(vBox);
  }

  void keyPressEvent(QKeyEvent *event) override {}
  void mousePressEvent(QMouseEvent* event) override {}
  void mouseReleaseEvent(QMouseEvent* event) override {}
  void mouseMoveEvent(QMouseEvent* event) override {}

  void reloadFont(const char* fileName) {
    _blFace.reset();
    if (_qtApplicationFontId != -1)
      QFontDatabase::removeApplicationFont(_qtApplicationFontId);

    BLArray<uint8_t> dataBuffer;
    if (BLFileSystem::readFile(fileName, dataBuffer) == BL_SUCCESS) {
      BLFontData fontData;
      if (fontData.createFromData(dataBuffer) == BL_SUCCESS) {
        _blFace.createFromData(fontData, 0);
      }

      QByteArray qtBuffer(reinterpret_cast<const char*>(dataBuffer.data()), dataBuffer.size());
      _qtApplicationFontId = QFontDatabase::addApplicationFontFromData(qtBuffer);
    }
  }

private Q_SLOTS:
  Q_SLOT void onStyleChanged(int index) { _canvas->updateCanvas(); }
  Q_SLOT void onRendererChanged(int index) { _canvas->setRendererType(_rendererSelect->itemData(index).toInt()); }

  void selectFile() {
    QString fileName = _fileSelected->text();
    QFileDialog dialog(this);

    if (!fileName.isEmpty())
      dialog.setDirectory(QFileInfo(fileName).absoluteDir().path());

    dialog.setAcceptMode(QFileDialog::AcceptOpen);
    dialog.setFileMode(QFileDialog::ExistingFile);
    dialog.setNameFilter(QString("Font File (*.ttf *.otf)"));
    dialog.setViewMode(QFileDialog::Detail);

    if (dialog.exec() == QDialog::Accepted) {
      fileName = dialog.selectedFiles()[0];
      _fileSelected->setText(fileName);
    }
  }

  void fileChanged(const QString&) {
    QByteArray fileNameUtf8 = _fileSelected->text().toUtf8();
    fileNameUtf8.append('\0');

    reloadFont(fileNameUtf8.constData());
    _canvas->updateCanvas();
  }

  void valueChanged(int value) {
    _canvas->updateCanvas();
  }

  void textChanged(const QString& text) {
    _canvas->updateCanvas();
  }

public:
  void onRenderB2D(BLContext& ctx) noexcept {
    ctx.fillAll(BLRgba32(0xFF000000));

    int styleId = _styleSelect->currentIndex();
    BLVar style;

    switch (styleId) {
      default:
      case 0: {
        style = BLRgba32(0xFFFFFFFF);
        break;
      }

      case 1: {
        double w = _canvas->blImage.width();
        double h = _canvas->blImage.height();

        BLGradient g(BLLinearGradientValues(0, 0, w, h));
        g.addStop(0.0, BLRgba32(0xFFFF0000));
        g.addStop(0.5, BLRgba32(0xFFAF00AF));
        g.addStop(1.0, BLRgba32(0xFF0000FF));

        style = g;
        break;
      }

      case 2: {
        double w = _canvas->blImage.width();
        double h = _canvas->blImage.height();
        double r = blMin(w, h);

        BLGradient g(BLRadialGradientValues(w * 0.5, h * 0.5, w * 0.5, h * 0.5, r * 0.5));
        g.addStop(0.0, BLRgba32(0xFFFF0000));
        g.addStop(0.5, BLRgba32(0xFFAF00AF));
        g.addStop(1.0, BLRgba32(0xFF0000FF));

        style = g;
        break;
      }

      case 3: {
        double w = _canvas->blImage.width();
        double h = _canvas->blImage.height();

        BLGradient g(BLConicGradientValues(w * 0.5, h * 0.5, 0.0));
        g.addStop(0.00, BLRgba32(0xFFFF0000));
        g.addStop(0.33, BLRgba32(0xFFAF00AF));
        g.addStop(0.66, BLRgba32(0xFF0000FF));
        g.addStop(1.00, BLRgba32(0xFFFF0000));

        style = g;
        break;
      }
    }

    BLFont font;
    font.createFromFace(_blFace, _slider->value());

    // Qt uses UTF-16 strings, Blend2D can process them natively.
    QString text = _text->text();
    PerformanceTimer timer;
    timer.start();
    ctx.fillUtf16Text(BLPoint(10, 10 + font.size()), font, reinterpret_cast<const uint16_t*>(text.constData()), text.length(), style);
    timer.stop();

    if (_otDebug->checkState() == Qt::Checked) {
      BLGlyphBuffer gb;
      BLString output;
      gb.setDebugSink(debugGlyphBufferSink, &output);
      gb.setUtf16Text(reinterpret_cast<const uint16_t*>(text.constData()), text.length());
      font.shape(gb);

      BLFont smallFont;
      smallFont.createFromFace(_blFace, 22.0f);
      BLFontMetrics metrics = smallFont.metrics();

      size_t i = 0;
      BLPoint pos(10, 10 + font.size() * 1.2 + smallFont.size());
      while (i < output.size()) {
        size_t end = blMin(output.indexOf('\n', i), output.size());

        BLRgba32 color = BLRgba32(0xFFFFFFFF);
        if (end - i > 0 && output.data()[i] == '[')
          color = BLRgba32(0xFFFFFF00);

        ctx.fillUtf8Text(pos, smallFont, output.data() + i, end - i, color);
        pos.y += metrics.ascent + metrics.descent;
        i = end + 1;
      }
    }

    _updateTitle(timer.duration());
  }

  void onRenderQt(QPainter& ctx) noexcept {
    ctx.fillRect(0, 0, _canvas->width(), _canvas->height(), QColor(0, 0, 0));

    if (_qtApplicationFontId == -1)
      return;

    int styleId = _styleSelect->currentIndex();
    QBrush brush;

    switch (styleId) {
      default:
      case 0: {
        brush = QColor(255, 255, 255);
        break;
      }

      case 1: {
        double w = _canvas->blImage.width();
        double h = _canvas->blImage.height();

        QLinearGradient g(qreal(0), qreal(0), qreal(w), qreal(h));
        g.setColorAt(0.0f, QColor(0xFF, 0x00, 0x00));
        g.setColorAt(0.5f, QColor(0xAF, 0x00, 0xAF));
        g.setColorAt(1.0f, QColor(0x00, 0x00, 0xFF));

        brush = QBrush(g);
        break;
      }

      case 2: {
        double w = _canvas->blImage.width();
        double h = _canvas->blImage.height();
        double r = blMin(w, h);

        QRadialGradient g(qreal(w * 0.5), qreal(h * 0.5), qreal(r * 0.5), qreal(w * 0.5), qreal(h * 0.5));
        g.setColorAt(0.0f, QColor(0xFF, 0x00, 0x00));
        g.setColorAt(0.5f, QColor(0xAF, 0x00, 0xAF));
        g.setColorAt(1.0f, QColor(0x00, 0x00, 0xFF));

        brush = QBrush(g);
        break;
      }

      case 3: {
        double w = _canvas->blImage.width();
        double h = _canvas->blImage.height();

        QConicalGradient g(qreal(w * 0.5), qreal(h * 0.5), 0.0);
        g.setColorAt(0.00f, QColor(0xFF, 0x00, 0x00));
        g.setColorAt(0.66f, QColor(0xAF, 0x00, 0xAF));
        g.setColorAt(0.33f, QColor(0x00, 0x00, 0xFF));
        g.setColorAt(1.00f, QColor(0xFF, 0x00, 0x00));

        brush = QBrush(g);
        break;
      }
    }

    QStringList families = QFontDatabase::applicationFontFamilies(_qtApplicationFontId);
    QFont font = QFont(families[0]);
    font.setPixelSize(_slider->value());
    font.setHintingPreference(QFont::PreferNoHinting);
    ctx.setFont(font);

    QPen pen(brush, 1.0f);
    ctx.setPen(pen);

    PerformanceTimer timer;
    timer.start();
    ctx.drawText(QPointF(10, 10 + font.pixelSize()), _text->text());
    timer.stop();
    _updateTitle(timer.duration());
  }

  void _updateTitle(double duration) {
    char buf[256];
    snprintf(buf, 256, "Text Sample [Size %dpx TextRenderTime %0.3fms]", int(_slider->value()), duration);

    QString title = QString::fromUtf8(buf);
    if (title != windowTitle())
      setWindowTitle(title);
  }
};

int main(int argc, char *argv[]) {
  QApplication app(argc, argv);
  MainWindow win;

  win.resize(QSize(580, 520));
  win.show();

  return app.exec();
}

#include "bl_text_demo.moc"

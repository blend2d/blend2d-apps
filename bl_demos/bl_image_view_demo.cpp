#include <blend2d.h>

#include "bl_qt_headers.h"
#include "bl_qt_canvas.h"

class MainWindow : public QWidget {
  Q_OBJECT

public:
  // Widgets.
  QLineEdit* _fileSelected {};
  QPushButton* _fileSelectButton {};
  QCheckBox* _animateCheck {};
  QCheckBox* _backgroundCheck {};
  QPushButton* _nextFrameButton {};
  QBLCanvas* _canvas {};

  BLArray<uint8_t> _imageFileData;
  BLImageDecoder _imageDecoder;
  BLImageInfo _loadedImageInfo {};
  BLImage _loadedImage;
  BLString _errorMessage;

  QTimer _timer;

  MainWindow() {
    QVBoxLayout* vBox = new QVBoxLayout();
    vBox->setContentsMargins(0, 0, 0, 0);
    vBox->setSpacing(0);

    QGridLayout* grid = new QGridLayout();
    grid->setContentsMargins(5, 5, 5, 5);
    grid->setSpacing(5);

    _fileSelected = new QLineEdit("");
    _fileSelectButton = new QPushButton("Select...");
    _animateCheck = new QCheckBox("Animate");
    _animateCheck->setChecked(true);
    _backgroundCheck = new QCheckBox("White");
    _nextFrameButton = new QPushButton("Next");
    _canvas = new QBLCanvas();

    connect(_fileSelectButton, SIGNAL(clicked()), SLOT(selectFile()));
    connect(_fileSelected, SIGNAL(textChanged(const QString&)), SLOT(fileChanged(const QString&)));
    connect(&_timer, SIGNAL(timeout()), this, SLOT(onTimer()));
    connect(_nextFrameButton, SIGNAL(clicked()), SLOT(onNextFrame()));
    connect(_backgroundCheck, SIGNAL(clicked()), SLOT(onRedraw()));

    _canvas->onRenderB2D = std::bind(&MainWindow::onRenderB2D, this, std::placeholders::_1);

    grid->addWidget(new QLabel("Image:"), 0, 0);
    grid->addWidget(_fileSelected, 0, 1, 1, 3);
    grid->addWidget(_fileSelectButton, 0, 4);
    grid->addWidget(_animateCheck, 0, 5);
    grid->addWidget(_backgroundCheck, 0, 6);
    grid->addWidget(_nextFrameButton, 0, 7);

    vBox->addItem(grid);
    vBox->addWidget(_canvas);

    setLayout(vBox);
    _timer.setInterval(50);
  }

  void keyPressEvent(QKeyEvent *event) override {}
  void mousePressEvent(QMouseEvent* event) override {}
  void mouseReleaseEvent(QMouseEvent* event) override {}
  void mouseMoveEvent(QMouseEvent* event) override {}

  bool createDecoder(const char* fileName) {
    _imageFileData.reset();
    _imageDecoder.reset();
    _loadedImageInfo.reset();
    _loadedImage.reset();
    _errorMessage.clear();

    if (BLFileSystem::readFile(fileName, _imageFileData) != BL_SUCCESS) {
      _errorMessage.assign("Failed to read the file specified");
      return false;
    }

    BLImageCodec codec;
    if (codec.findByData(_imageFileData) != BL_SUCCESS) {
      _errorMessage.assign("Failed to find a codec for the given file");
      return false;
    }

    if (codec.createDecoder(&_imageDecoder) != BL_SUCCESS) {
      _errorMessage.assign("Failed to create a decoder for the given file");
      return false;
    }

    if (_imageDecoder.readInfo(_loadedImageInfo, _imageFileData) != BL_SUCCESS) {
      _errorMessage.assign("Failed to read image information");
      return false;
    }

    return true;
  }

  bool readFrame() {
    if (_imageDecoder.readFrame(_loadedImage, _imageFileData) != BL_SUCCESS) {
      _errorMessage.assign("Failed to decode the image");
      return false;
    }

    return true;
  }

  void loadImage(const char* fileName) {
    if (createDecoder(fileName)) {
      if (readFrame()) {
        if (_loadedImageInfo.frameCount > 1u) {
          _timer.start();
        }
      }
    }

    _updateTitle();
  }

  Q_SLOT void selectFile() {
    QString fileName = _fileSelected->text();
    QFileDialog dialog(this);

    if (!fileName.isEmpty())
      dialog.setDirectory(QFileInfo(fileName).absoluteDir().path());

    dialog.setAcceptMode(QFileDialog::AcceptOpen);
    dialog.setFileMode(QFileDialog::ExistingFile);
    dialog.setNameFilter(QString("Image File (*.apng *.bmp *.jpeg *.jpg *.png *.qoi)"));
    dialog.setViewMode(QFileDialog::Detail);

    if (dialog.exec() == QDialog::Accepted) {
      fileName = dialog.selectedFiles()[0];
      _fileSelected->setText(fileName);
    }
  }

  Q_SLOT void fileChanged(const QString&) {
    QByteArray fileNameUtf8 = _fileSelected->text().toUtf8();
    fileNameUtf8.append('\0');

    loadImage(fileNameUtf8.constData());
    _canvas->updateCanvas();
  }

  Q_SLOT void onTimer() {
    if (!_animateCheck->isChecked()) {
      return;
    }

    if (readFrame()) {
      _canvas->updateCanvas();
    }
    else {
      _timer.stop();
    }
  }

  Q_SLOT void onNextFrame() {
    if (_loadedImageInfo.frameCount > 1u) {
      if (readFrame()) {
        _canvas->updateCanvas();
      }
    }
  }

  Q_SLOT void onRedraw() {
    _canvas->updateCanvas();
  }

  void onRenderB2D(BLContext& ctx) noexcept {
    BLRgba32 background = _backgroundCheck->isChecked() ? BLRgba32(0xFFFFFFFFu) : BLRgba32(0xFF000000u);

    ctx.fillAll(background);
    ctx.blitImage(BLPointI(0, 0), _loadedImage);
  }

  void _updateTitle() {
    BLString s;

    if (!_errorMessage.empty()) {
      s.assignFormat("Load ERROR=%s", _errorMessage.data());
    }
    else {
      s.assignFormat("Image Size=[%dx%d] Format=%s Depth=%u Compression=%s Frames=%llu",
        _loadedImage.width(),
        _loadedImage.height(),
        _loadedImageInfo.format,
        _loadedImageInfo.depth,
        _loadedImageInfo.compression,
        (unsigned long long)_loadedImageInfo.frameCount);
    }

    setWindowTitle(QString::fromUtf8(s.data()));
  }
};

int main(int argc, char *argv[]) {
  QApplication app(argc, argv);
  MainWindow win;

  win.resize(QSize(580, 520));
  win.show();

  return app.exec();
}

#include "bl_image_view_demo.moc"

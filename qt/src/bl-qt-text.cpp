#include <QtGui>
#include <QtWidgets>

#include <blend2d.h>
#include "qblcanvas.h"
#include "bl-qt-static.h"

class MainWindow : public QWidget {
  Q_OBJECT

public:
  // Widgets.
  QLineEdit* _fileSelected;
  QPushButton* _fileSelectButton;
  QSlider* _slider;
  QLineEdit* _text;
  QBLCanvas* _canvas;

  // Canvas data.
  BLFontFace _face;

  MainWindow() {
    QVBoxLayout* vBox = new QVBoxLayout();
    vBox->setContentsMargins(0, 0, 0, 0);
    vBox->setSpacing(0);

    QGridLayout* grid = new QGridLayout();
    grid->setContentsMargins(5, 5, 5, 5);
    grid->setSpacing(5);

    _fileSelected = new QLineEdit("");
    _fileSelectButton = new QPushButton("Select Font ...");
    _slider = new QSlider();
    _canvas = new QBLCanvas();

    _slider->setOrientation(Qt::Horizontal);
    _slider->setMinimum(5);
    _slider->setMaximum(200);
    _slider->setSliderPosition(20);

    _text = new QLineEdit();
    _text->setText(QString("Test"));

    connect(_fileSelectButton, SIGNAL(clicked()), SLOT(selectFile()));
    connect(_fileSelected, SIGNAL(textChanged(const QString&)), SLOT(fileChanged(const QString&)));
    connect(_slider, SIGNAL(valueChanged(int)), SLOT(valueChanged(int)));
    connect(_text, SIGNAL(textChanged(const QString&)), SLOT(textChanged(const QString&)));
    _canvas->onRenderB2D = std::bind(&MainWindow::onRender, this, std::placeholders::_1);

    grid->addWidget(_fileSelected, 0, 0);
    grid->addWidget(_fileSelectButton, 0, 1);
    grid->addWidget(_slider, 1, 0, 1, 2);
    grid->addWidget(_text, 2, 0, 1, 2);
    vBox->addItem(grid);
    vBox->addWidget(_canvas);
    setLayout(vBox);
  }

  void keyPressEvent(QKeyEvent *event) override {}
  void mousePressEvent(QMouseEvent* event) override {}
  void mouseReleaseEvent(QMouseEvent* event) override {}
  void mouseMoveEvent(QMouseEvent* event) override {}

private Q_SLOTS:
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
    QByteArray fileName = _fileSelected->text().toUtf8();

    BLFontFace face;
    face.createFromFile(fileName.constData());

    if (face.isValid()) {
      _face = face;
      _canvas->updateCanvas();
    }
  }

  void valueChanged(int value) {
    _canvas->updateCanvas();
  }

  void textChanged(const QString& text) {
    _canvas->updateCanvas();
  }

public:
  void onRender(BLContext& ctx) noexcept {
    BLFont font;
    font.createFromFace(_face, _slider->value());

    ctx.setFillStyle(BLRgba32(0xFF000000));
    ctx.fillAll();

    // Qt uses UTF-16 strings, Blend2D can process them natively.
    QString text = _text->text();
    ctx.setFillStyle(BLRgba32(0xFFFFFFFF));
    ctx.fillUtf16Text(BLPoint(10, 10 + font.size()), font, reinterpret_cast<const uint16_t*>(text.constData()), text.length());
  }
};

int main(int argc, char *argv[]) {
  QApplication app(argc, argv);
  MainWindow win;

  win.resize(QSize(580, 520));
  win.show();

  return app.exec();
}

#include "bl-qt-text.moc"

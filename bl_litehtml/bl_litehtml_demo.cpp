#include <stdio.h>

#include <QtGui>
#include <QtWidgets>

#include "bl_litehtml_default_page.h"
#include "bl_litehtml_view.h"

class MainWindow : public QWidget {
  Q_OBJECT

  BLLiteHtmlView htmlView;

public:
  MainWindow() {
    QBoxLayout* vBox = new QHBoxLayout();

    vBox->setContentsMargins(0, 0, 0, 0);
    vBox->setSpacing(0);
    vBox->setDirection(QBoxLayout::TopToBottom);

    vBox->addWidget(&htmlView);
    setLayout(vBox);

    setWindowTitle(QLatin1String("LiteHtml & Blend2D"));
  }

  ~MainWindow() override {}

  void setUrl(const char* url) {
    htmlView.setUrl(url);
  }

  void setContent(BLStringView content) {
    htmlView.setContent(content);
  }
};

int main(int argc, char *argv[]) {
  QApplication app(argc, argv);
  MainWindow win;

  win.resize(QSize(1040, 720));
  win.show();

  if (argc > 1) {
    win.setUrl(argv[1]);
  }
  else {
    const char* content = get_default_page_content();
    win.setContent(BLStringView{content, strlen(content)});
  }

  return app.exec();
}

#include "bl_litehtml_demo.moc"

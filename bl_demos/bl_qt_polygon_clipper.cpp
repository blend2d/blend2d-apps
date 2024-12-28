#include "bl_qt_headers.h"
#include "bl_qt_canvas.h"

#include <QPolygonF>

#include <stdlib.h>

static const double PI = 3.14159265359;

class MainWindow : public QWidget {
  Q_OBJECT

public:
  enum OperationType : uint32_t {
      Union = 0,
      Intersection = 1,
      Difference = 2,
      SymmetricDifference = 3,
  };

  QTimer _timer;
  QBLCanvas _canvas;
  QComboBox _rendererSelect;
  QComboBox _operationSelect;
  QCheckBox _limitFpsCheck;
  QPointF _mousePt;
  OperationType _operationType = Intersection;

  MainWindow() {
    QVBoxLayout* vBox = new QVBoxLayout();
    vBox->setContentsMargins(0, 0, 0, 0);
    vBox->setSpacing(0);

    QGridLayout* grid = new QGridLayout();
    grid->setContentsMargins(5, 5, 5, 5);
    grid->setSpacing(5);

    QBLCanvas::initRendererSelectBox(&_rendererSelect);
    _limitFpsCheck.setText(QLatin1String("Limit FPS"));

    _operationSelect.addItem("Union", QVariant(int(Union)));
    _operationSelect.addItem("Intersection", QVariant(int(Intersection)));
    _operationSelect.addItem("Difference", QVariant(int(Difference)));
    _operationSelect.addItem("Symmetric Difference", QVariant(int(SymmetricDifference)));
    _operationSelect.setCurrentIndex(_operationSelect.findData(int(_operationType)));

    connect(&_rendererSelect, SIGNAL(currentIndexChanged(int)), SLOT(onRendererChanged(int)));
    connect(&_limitFpsCheck, SIGNAL(stateChanged(int)), SLOT(onLimitFpsChanged(int)));
    connect(&_operationSelect, SIGNAL(currentIndexChanged(int)), SLOT(onOperationChanged(int)));

    grid->addWidget(new QLabel("Renderer:"), 0, 0, Qt::AlignRight);
    grid->addWidget(&_rendererSelect, 0, 1);

    grid->addWidget(new QLabel("Operation:"), 0, 2, Qt::AlignRight);
    grid->addWidget(&_operationSelect, 0, 3);

    grid->addItem(new QSpacerItem(0, 0, QSizePolicy::Expanding), 0, 4);
    grid->addWidget(&_limitFpsCheck, 0, 5);

    _canvas.onRenderB2D = std::bind(&MainWindow::onRenderB2D, this, std::placeholders::_1);
    _canvas.onRenderQt = std::bind(&MainWindow::onRenderQt, this, std::placeholders::_1);
    _canvas.onMouseEvent = std::bind(&MainWindow::onMouseEvent, this, std::placeholders::_1);

    vBox->addLayout(grid);
    vBox->addWidget(&_canvas);
    setLayout(vBox);

    connect(&_timer, SIGNAL(timeout()), this, SLOT(onTimer()));
    onInit();
  }

  void showEvent(QShowEvent* event) override { _timer.start(); }
  void hideEvent(QHideEvent* event) override { _timer.stop(); }

  void onInit() {
    _updateTitle();
    _limitFpsCheck.setChecked(true);
  }

  Q_SLOT void onRendererChanged(int index) { _canvas.setRendererType(_rendererSelect.itemData(index).toInt()); }
  Q_SLOT void onOperationChanged(int index) { _operationType = (OperationType)_operationSelect.itemData(index).toInt(); }
  Q_SLOT void onLimitFpsChanged(int value) { _timer.setInterval(value ? 1000 / 120 : 0); }
  Q_SLOT void onToggleRenderer() { _rendererSelect.setCurrentIndex(_rendererSelect.currentIndex() ^ 1); }

  Q_SLOT void onTimer() {
    _canvas.updateCanvas(true);
    _updateTitle();
  }

  void onRenderB2D(BLContext& ctx) noexcept {
    ctx.fillAll(BLRgba32(0xFFFFFFFFu));

      std::vector<QPolygonF> subjectPolygons = getSubjectPolygons();
      std::vector<QPolygonF> clippingPolygons = getClippingPolygons();

      BLPath subjectPath;
      for (const QPolygonF& subjectPolygon : subjectPolygons)
      {
          subjectPath.moveTo(subjectPolygon.front().x(), subjectPolygon.front().y());
          for (size_t i = 1; i < subjectPolygon.size(); ++i)
              subjectPath.lineTo(subjectPolygon[i].x(), subjectPolygon[i].y());
      }

      BLPath clippingPath;
      for (const QPolygonF& clippingPolygon : clippingPolygons)
      {
          clippingPath.moveTo(clippingPolygon.front().x(), clippingPolygon.front().y());
          for (size_t i = 1; i < clippingPolygon.size(); ++i)
              clippingPath.lineTo(clippingPolygon[i].x(), clippingPolygon[i].y());
      }

      BLPath resultPath;
      switch (_operationType) {
      case Union:
          resultPath = subjectPath.united(clippingPath);
          break;
      case Intersection:
          resultPath = subjectPath.intersected(clippingPath);
          break;
      case Difference:
          resultPath = subjectPath.subtracted(clippingPath);
          break;
      case SymmetricDifference:
          resultPath = subjectPath.symmetricDifference(clippingPath);
          break;
      }
      ctx.setFillRule(BL_FILL_RULE_EVEN_ODD);
      ctx.fillPath(subjectPath, BLRgba32(0x6400FF00));
      ctx.fillPath(clippingPath, BLRgba32(0x64FFFF00));
      ctx.fillPath(resultPath, BLRgba32(0xFFFF0000));
  }

  void onRenderQt(QPainter& ctx) noexcept {
      ctx.fillRect(_canvas.rect(), Qt::white);

      std::vector<QPolygonF> subjectPolygons = getSubjectPolygons();
      std::vector<QPolygonF> clippingPolygons = getClippingPolygons();

      QPainterPath subjectPath;
      for (const QPolygonF& subjectPolygon : subjectPolygons)
      {
          subjectPath.addPolygon(subjectPolygon);
      }

      QPainterPath clippingPath;
      for (const QPolygonF& clippingPolygon : clippingPolygons)
      {
          clippingPath.addPolygon(clippingPolygon);
      }

      QPainterPath resultPath;
      switch (_operationType) {
      case Union:
          resultPath = subjectPath.united(clippingPath);
          break;
      case Intersection:
          resultPath = subjectPath.intersected(clippingPath);
          break;
      case Difference:
          resultPath = subjectPath.subtracted(clippingPath);
          break;
      case SymmetricDifference:
      {
          QPainterPath up = subjectPath.united(clippingPath);
          QPainterPath ip = subjectPath.intersected(clippingPath);
          resultPath = up.subtracted(ip);
          break;
      }
      }

      ctx.fillPath(subjectPath, QColor(0, 255, 0, 100));
      ctx.fillPath(clippingPath, QColor(255, 255, 0, 100));
      ctx.fillPath(resultPath, QColor(255, 0, 0, 255));
  }

  std::vector<QPolygonF> getClippingPolygons() const noexcept {
      QPainterPath path;
      path.addEllipse(_mousePt, _canvas.width() * 0.2, _canvas.height() * 0.2);
      return { path.toFillPolygon() };
  }

  std::vector<QPolygonF> getSubjectPolygons() const noexcept {
      std::vector<QPolygonF> polygons;

      double width = _canvas.width();
      double height = _canvas.height();

      double w4 = width / 4.0;
      double h4 = height / 4.0;

      for (int i = 0; i < 4; ++i)
      {
          QPointF center(w4 * i + w4 * 0.5, h4 * 0.5);
          QRectF rect(0.0, 0.0, w4 * (0.8 - i * 0.07), h4 * (0.8 - i * 0.07));
          rect.moveCenter(center);

          QPolygonF polygon;
          polygon << rect.topLeft();
          polygon << rect.topRight();
          polygon << rect.bottomRight();
          polygon << rect.bottomLeft();
          polygon << rect.topLeft();
          polygons.push_back(polygon);
      }

      for (int i = 0; i < 4; ++i)
      {
          QPointF center(w4 * i + w4 * 0.5, h4 * 1.5);
          QRectF rect(0.0, 0.0, w4 * (0.8 - i * 0.07), h4 * (0.8 - i * 0.07));
          rect.moveCenter(center);

          QPolygonF polygon;
          polygon << rect.topLeft();
          polygon << rect.topRight();
          polygon << rect.bottomRight();
          polygon << rect.bottomLeft();
          polygon << rect.topLeft();
          polygons.push_back(polygon);

          rect.setWidth(rect.width() * 0.8);
          rect.setHeight(rect.height() * 0.8);
          rect.moveCenter(center);

          polygon.clear();
          polygon << rect.topLeft();
          polygon << rect.topRight();
          polygon << rect.bottomRight();
          polygon << rect.bottomLeft();
          polygon << rect.topLeft();
          polygons.push_back(polygon);
      }

      for (int i = 0; i < 4; ++i)
      {
          QPointF center(w4 * i + w4 * 0.5, h4 * 2.5);
          QRectF rect(0.0, 0.0, w4 * (0.8 - i * 0.07), h4 * (0.8 - i * 0.07));
          rect.moveCenter(center);

          QPainterPath path;
          path.addEllipse(rect);
          polygons.push_back(path.toFillPolygon());
      }

      for (int i = 0; i < 4; ++i)
      {
          QPointF center(w4 * i + w4 * 0.5, h4 * 3.5);
          QRectF rect(0.0, 0.0, w4 * (0.8 - i * 0.07), h4 * (0.8 - i * 0.07));
          rect.moveCenter(center);

          QPainterPath path;
          path.addEllipse(rect);
          polygons.push_back(path.toFillPolygon());

          rect.setWidth(rect.width() * 0.8);
          rect.setHeight(rect.height() * 0.8);
          rect.moveCenter(center);

          path.clear();
          path.addEllipse(rect);
          polygons.push_back(path.toFillPolygon());
      }

      return polygons;
  }

  void onMouseEvent(QMouseEvent* event) {
      if (event->type() == QEvent::MouseMove) {
          double mx = event->position().x();
          double my = event->position().y();
          _mousePt = QPointF(mx, my);
      }
  }

  void _updateTitle() {
    char buf[256];
    snprintf(buf, 256, "Polygon Clipper Sample [%dx%d] [AvgTime=%.2fms FPS=%.1f]",
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

  win.setMinimumSize(QSize(400, 320));
  win.resize(QSize(580, 520));
  win.show();

  return app.exec();
}

#include "bl_qt_polygon_clipper.moc"

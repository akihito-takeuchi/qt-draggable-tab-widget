// Copyright (c) 2019 Akihito Takeuchi
// Distributed under the MIT License : http://opensource.org/licenses/MIT

#include "draggabletabwidget.h"

#include <QTabBar>
#include <QPixmap>
#include <QPainter>
#include <QMouseEvent>
#include <QDrag>
#include <QMimeData>
#include <QWindow>
#include <QApplication>

using TabInfo = DraggableTabWidget::TabInfo;

namespace {

// class DraggableTabBar =======================================================

class DraggableTabBar : public QTabBar {
  Q_OBJECT
 public:
  DraggableTabBar(QWidget* parent = nullptr);
  ~DraggableTabBar() = default;

 protected:
  void mousePressEvent(QMouseEvent* event) override;
  void mouseReleaseEvent(QMouseEvent* event) override;
  void mouseMoveEvent(QMouseEvent* event) override;
  void dragEnterEvent(QDragEnterEvent* event) override;
  void dropEvent(QDropEvent* event) override;
  bool eventFilter(QObject* watched, QEvent* event) override;

 signals:
  void createWindowRequested(const QRect& win_rect,
                             const TabInfo& tab_info);

 private:
  void startDrag();
  bool dispatchEvent(QEvent* event);
  void sendButtonRelease(QObject* target = nullptr);
  void startTabMove();
  DraggableTabWidget* parentWidget_() const;
  
  QPoint click_point_;

  static TabInfo drag_tab_info_;
  static bool dragging_;
};

TabInfo DraggableTabBar::drag_tab_info_;
bool DraggableTabBar::dragging_ = false;

DraggableTabBar::DraggableTabBar(QWidget* parent)
    : QTabBar(parent) {
  setAcceptDrops(true);
  qApp->installEventFilter(this);
}

void DraggableTabBar::mousePressEvent(QMouseEvent* event) {
  if (event->button() == Qt::LeftButton) {
    auto current_index = tabAt(event->pos());
    auto parent = parentWidget_();
    parent->setCurrentIndex(current_index);
    drag_tab_info_ = TabInfo(
        parent->currentWidget(), tabText(current_index), tabIcon(current_index),
        tabToolTip(current_index), tabWhatsThis(current_index));
    dragging_ = false;
    click_point_ = event->globalPos() - window()->pos();
  }
  QTabBar::mousePressEvent(event);
}

void DraggableTabBar::mouseReleaseEvent(QMouseEvent* event) {
  click_point_ = QPoint();
  dragging_ = false;
  QTabBar::mouseReleaseEvent(event);
}

void DraggableTabBar::mouseMoveEvent(QMouseEvent* event) {
  if (click_point_.isNull())
    return;

  if (geometry().contains(event->pos())) {
    QTabBar::mouseMoveEvent(event);
  } else if (!dragging_) {
    // start dragging
    startDrag();
    event->accept();
  } else {
    QTabBar::mouseMoveEvent(event);
  }
}
  
void DraggableTabBar::dragEnterEvent(QDragEnterEvent* event) {
  // When mouse cursor is back to source widget, cancel drag
  if (geometry().contains(event->pos())
      && event->source() == this) {
    QDrag::cancel();
    event->accept();
    return;
  }
  event->acceptProposedAction();
  sendButtonRelease(event->source());
}

void DraggableTabBar::dropEvent(QDropEvent* event) {
  event->acceptProposedAction();
  startTabMove();
}

bool DraggableTabBar::eventFilter(QObject* watched, QEvent* event) {
  if (watched == static_cast<QObject*>(window()->windowHandle()))
    return dispatchEvent(event);
  return false;
}

void DraggableTabBar::startDrag() {
  dragging_ = true;
  auto parent = parentWidget_();
  auto idx = parent->indexOf(drag_tab_info_.widget());
  parent->removeTab(idx);
  if (count() == 0)
    window()->hide();
  auto drag = new QDrag(this);
  auto data = new QMimeData;
  drag->setMimeData(data);
  drag->setPixmap(drag_tab_info_.widget()->grab());
  auto drop_action = drag->exec(Qt::MoveAction);
  if (drop_action == Qt::IgnoreAction) {
    if (drag->target() == this) {
      // Back to normal move event
      sendButtonRelease();
      startTabMove();
      dragging_ = false;
    } else {
      // Drop out. Create new window and complete mouse move
      auto global_pos = QCursor::pos();
      auto win_rect = parentWidget()->geometry();
      win_rect.moveTo(global_pos);
      emit createWindowRequested(win_rect, drag_tab_info_);
      dragging_ = false;
      sendButtonRelease();
    }
  }
  if (count() == 0)
    window()->deleteLater();
  drag->deleteLater();
  data->deleteLater();
}

bool DraggableTabBar::dispatchEvent(QEvent* event) {
  QList<QEvent::Type> dispatch_types;
  dispatch_types << QEvent::MouseMove << QEvent::MouseButtonRelease;
  
  auto mouse_event = static_cast<QMouseEvent*>(event);
  auto global_pos = mouse_event->globalPos();
  auto pos = mapFromGlobal(global_pos);

  if (!dispatch_types.contains(event->type()))
    return false;
  if (event->type() == QEvent::MouseMove) {
    if (dragging_)
      return false;
    if (!geometry().contains(pos))
      return false;
  }
  if (event->type() == QEvent::MouseButtonRelease && click_point_.isNull())
    return false;

  QMouseEvent new_event(
      mouse_event->type(), pos, global_pos,
      mouse_event->button(), mouse_event->buttons(), mouse_event->modifiers());

  switch (mouse_event->type()) {
    case QEvent::MouseMove:
      mouseMoveEvent(&new_event);
      break;
    case QEvent::MouseButtonRelease:
      mouseReleaseEvent(&new_event);
      break;
    default:
      break;
  }
  return true;
}

void DraggableTabBar::sendButtonRelease(QObject* target) {
  auto global_pos = QCursor::pos();
  auto pos = mapFromGlobal(global_pos);
  auto modifiers = QApplication::keyboardModifiers();

  auto release_event = new QMouseEvent(
      QEvent::MouseButtonRelease, pos, global_pos,
      Qt::LeftButton, Qt::LeftButton, modifiers);
  if (!target)
    target = this;
  QApplication::postEvent(target, release_event);
}

void DraggableTabBar::startTabMove() {
  auto global_pos = QCursor::pos();
  auto pos = mapFromGlobal(global_pos);
  auto modifiers = QApplication::keyboardModifiers();

  auto idx = tabAt(pos);
  auto parent = parentWidget_();
  parent->insertTab(
      idx,
      drag_tab_info_.widget(),
      drag_tab_info_.icon(),
      drag_tab_info_.text());
  parent->setTabToolTip(idx, drag_tab_info_.toolTip());
  parent->setTabWhatsThis(idx, drag_tab_info_.whatsThis());
  parent->setCurrentIndex(idx);

  auto press_event = new QMouseEvent(
      QEvent::MouseButtonPress, pos, global_pos,
      Qt::LeftButton, Qt::LeftButton, modifiers);
  QApplication::postEvent(this, press_event);
}

DraggableTabWidget* DraggableTabBar::parentWidget_() const {
  return qobject_cast<DraggableTabWidget*>(parent());
}

}  // namespace

// class DraggableTabWidget ====================================================

DraggableTabWidget::DraggableTabWidget(QWidget* parent)
    : QTabWidget(parent) {
  auto tab_bar = new DraggableTabBar(this);
  setTabBar(tab_bar);
  connect(tab_bar, &DraggableTabBar::createWindowRequested,
          this, &DraggableTabWidget::createNewWindow);
  setMovable(true);
}

DraggableTabWidget::~DraggableTabWidget() = default;

QWidget* DraggableTabWidget::createNewWindow(const QRect& win_rect,
                                             const TabInfo& tab_info) {
  auto new_window = new DraggableTabWidget;
  new_window->addTab(tab_info.widget(),
                     tab_info.icon(),
                     tab_info.text());
  new_window->setTabToolTip(0, tab_info.toolTip());
  new_window->setTabWhatsThis(0, tab_info.whatsThis());
  new_window->show();
  new_window->setGeometry(win_rect);
  return new_window;
}

#include "draggabletabwidget.moc"

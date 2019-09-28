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
  ~DraggableTabBar();

 protected:
  void mousePressEvent(QMouseEvent* event) override;
  void mouseReleaseEvent(QMouseEvent* event) override;
  void mouseMoveEvent(QMouseEvent* event) override;

 signals:
  void createWindowRequested(const QRect& win_rect,
                             const TabInfo& tab_info);

 private:
  void startDrag();
  QMouseEvent* createMouseEvent(
      QEvent::Type type, const QPoint& pos = QPoint());
  void startTabMove();
  DraggableTabWidget* parentTabWidget() const;
  void destroyUnnecessaryWindow();
  void insertCurrentTabInfo(int idx);

  QPoint click_point_ = QPoint();
  bool can_start_drag_ = false;

  static bool initializing_drag_;
  static TabInfo drag_tab_info_;
  static QWidget* dragging_widget_;
  static QList<DraggableTabBar*> tab_bar_instances_;
  static Qt::WindowFlags org_window_flags_;
};

bool DraggableTabBar::initializing_drag_ = false;
TabInfo DraggableTabBar::drag_tab_info_;
QWidget* DraggableTabBar::dragging_widget_ = nullptr;
QList<DraggableTabBar*> DraggableTabBar::tab_bar_instances_;
Qt::WindowFlags DraggableTabBar::org_window_flags_;

DraggableTabBar::DraggableTabBar(QWidget* parent)
    : QTabBar(parent) {
  tab_bar_instances_ << this;
}

DraggableTabBar::~DraggableTabBar() {
  tab_bar_instances_.removeOne(this);
}

void DraggableTabBar::mousePressEvent(QMouseEvent* event) {
  if (event->button() == Qt::LeftButton) {
    auto current_index = tabAt(event->pos());
    auto parent = parentTabWidget();
    parent->setCurrentIndex(current_index);
    drag_tab_info_ = TabInfo(
        parent->currentWidget(), tabText(current_index), tabIcon(current_index),
        tabToolTip(current_index), tabWhatsThis(current_index));
    dragging_widget_ = nullptr;
    org_window_flags_ = drag_tab_info_.widget()->windowFlags();
    click_point_ = event->pos();
    can_start_drag_ = false;
    grabMouse();
  }
  QTabBar::mousePressEvent(event);
}

void DraggableTabBar::mouseReleaseEvent(QMouseEvent* event) {
  if (event->button() == Qt::LeftButton) {
    if (initializing_drag_) {
      if (parentTabWidget()->indexOf(drag_tab_info_.widget()) < 0) {
        dragging_widget_ = drag_tab_info_.widget();
        dragging_widget_->setParent(nullptr);
        dragging_widget_->setWindowFlags(Qt::FramelessWindowHint);
      } else {
        dragging_widget_ = window();
      }
      initializing_drag_ = false;
      dragging_widget_->window()->raise();
    } else {
      if (dragging_widget_) {
        dragging_widget_->setWindowFlags(org_window_flags_);
        auto win_rect = dragging_widget_->geometry();
        win_rect.moveTo(event->globalPos());
        auto idx = parentTabWidget()->indexOf(drag_tab_info_.widget());
        if (idx >= 0)
          parentTabWidget()->removeTab(idx);
        emit createWindowRequested(win_rect, drag_tab_info_);
        destroyUnnecessaryWindow();
      }
      dragging_widget_ = nullptr;
      drag_tab_info_ = TabInfo();
      releaseMouse();
    }
  }
  click_point_ = QPoint();
  can_start_drag_ = false;
  QTabBar::mouseReleaseEvent(event);
}

void DraggableTabBar::mouseMoveEvent(QMouseEvent* event) {
  if (!drag_tab_info_.widget())
    return;

  if (!can_start_drag_) {
    auto moved_length = (event->pos() - click_point_).manhattanLength();
    can_start_drag_ = moved_length > qApp->startDragDistance();
  }
  if (dragging_widget_) {
    for (auto& bar_inst : tab_bar_instances_) {
      auto bar_region  = bar_inst->visibleRegion();
      bar_region.translate(bar_inst->mapToGlobal(QPoint(0, 0)));
      if (bar_region.contains(event->globalPos())) {
        if (bar_inst == this) {
          startTabMove();
          event->accept();
          return;
        } else {
          releaseMouse();
          bar_inst->grabMouse();
          event->accept();
          return;
        }
      }
    }
  }
  auto widget_rect = geometry();
  widget_rect.moveTo(0, 0);
  if (widget_rect.contains(event->pos())) {
    QTabBar::mouseMoveEvent(event);
  } else if (!dragging_widget_ && can_start_drag_) {
    // start dragging
    startDrag();
    event->accept();
    return;
  }

  if (dragging_widget_) {
    dragging_widget_->move(event->globalPos() + QPoint(3, 3));
    dragging_widget_->show();
  }
}

void DraggableTabBar::startDrag() {
  if (count() > 1) {
    auto parent = parentTabWidget();
    auto idx = parent->indexOf(drag_tab_info_.widget());
    parent->removeTab(idx);
    drag_tab_info_.widget()->setParent(nullptr);
  }
  dragging_widget_ = nullptr;
  initializing_drag_ = true;
  auto release_event = createMouseEvent(
      QEvent::MouseButtonRelease, mapFromGlobal(QCursor::pos()));
  QApplication::postEvent(this, release_event);
}

QMouseEvent* DraggableTabBar::createMouseEvent(
    QEvent::Type type, const QPoint& pos) {
  QPoint global_pos;
  if (pos.isNull())
    global_pos = QCursor::pos();
  else
    global_pos = mapToGlobal(pos);
  auto modifiers = QApplication::keyboardModifiers();

  auto event = new QMouseEvent(
      type, pos, global_pos,
      Qt::LeftButton, Qt::LeftButton, modifiers);
  return event;
}

void DraggableTabBar::startTabMove() {
  auto global_pos = QCursor::pos();
  auto pos = mapFromGlobal(global_pos);

  dragging_widget_->setWindowFlags(org_window_flags_);
  if (drag_tab_info_.widget()->parent()) {
    auto parent = qobject_cast<DraggableTabWidget*>(
        drag_tab_info_.widget()->parentWidget()->parentWidget());
    auto idx = parent->indexOf(drag_tab_info_.widget());
    parent->removeTab(idx);
    parent->window()->hide();
  }
    
  auto idx = tabAt(pos);
  insertCurrentTabInfo(idx);
  dragging_widget_ = nullptr;
  drag_tab_info_ = TabInfo();

  auto press_event = createMouseEvent(
      QEvent::MouseButtonPress, tabRect(idx).center());
  QApplication::postEvent(this, press_event);
  destroyUnnecessaryWindow();
  window()->raise();
}

DraggableTabWidget* DraggableTabBar::parentTabWidget() const {
  return qobject_cast<DraggableTabWidget*>(parent());
}

void DraggableTabBar::destroyUnnecessaryWindow() {
  for (auto& bar_inst : tab_bar_instances_)
    if (bar_inst->count() == 0
        && !bar_inst->isVisible())
      bar_inst->parent()->deleteLater();
}

void DraggableTabBar::insertCurrentTabInfo(int idx) {
  auto parent = parentTabWidget();
  parent->insertTab(
      idx,
      drag_tab_info_.widget(),
      drag_tab_info_.icon(),
      drag_tab_info_.text());
  parent->setTabToolTip(idx, drag_tab_info_.toolTip());
  parent->setTabWhatsThis(idx, drag_tab_info_.whatsThis());
  parent->setCurrentWidget(drag_tab_info_.widget());
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

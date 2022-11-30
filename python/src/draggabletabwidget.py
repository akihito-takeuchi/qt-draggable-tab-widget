# Copyright (c) 2019 Akihito Takeuchi
# Distributed under the MIT License : http://opensource.org/licenses/MIT

from PyQt5 import QtWidgets, QtGui, QtCore

Signal = QtCore.pyqtSignal
Slot = QtCore.pyqtSlot

class TabInfo:
    def __init__(self, widget = None, text = None, icon = None,
                 tool_tip = None, whats_this = None):
        self.widget = widget
        self.text = text
        self.icon = icon
        self.tool_tip = tool_tip
        self.whats_this = whats_this

class DraggableTabWidget(QtWidgets.QTabWidget):
    tab_widget_instances_ = []

    def __init__(self, parent = None):
        super().__init__(parent)
        tab_bar = DraggableTabBar(self)
        self.setTabBar(tab_bar)
        tab_bar.createWindowRequested.connect(self.createNewWindow)
        self.setMovable(True)
        DraggableTabWidget.tab_widget_instances_.append(self)

    def event(self, event):
        if event.type() == QtCore.QEvent.DeferredDelete:
            DraggableTabWidget.tab_widget_instances_.remove(self)
        return super().event(event)

    @Slot(QtCore.QRect, TabInfo)
    def createNewWindow(self, win_rect, tab_info):
        new_window = self.__class__()
        new_window.addTab(
            tab_info.widget,
            tab_info.icon,
            tab_info.text)
        new_window.setTabToolTip(0, tab_info.tool_tip)
        new_window.setTabWhatsThis(0, tab_info.whats_this)
        new_window.show()
        new_window.setGeometry(win_rect)
        return new_window

class DraggableTabBar(QtWidgets.QTabBar):
    createWindowRequested = Signal(QtCore.QRect, TabInfo)

    initializing_drag_ = False
    drag_tab_info_ = TabInfo()
    dragging_widget_ = None

    def __init__(self, parent = None):
        super().__init__(parent)
        self.click_point = QtCore.QPoint()
        self.can_start_drag = False

    def mousePressEvent(self, event):
        cls = DraggableTabBar
        if event.button() == QtCore.Qt.LeftButton:
            current_index = self.tabAt(event.pos())
            parent = self.parent()
            parent.setCurrentIndex(current_index)
            current_widget = parent.currentWidget()
            cls.drag_tab_info_ = TabInfo(
                current_widget, self.tabText(current_index),
                self.tabIcon(current_index), self.tabToolTip(current_index),
                self.tabWhatsThis(current_index))
            cls.dragging_widget_ = None
            self.click_point = event.pos()
            self.can_start_drag = False
            self.grabMouse()
        super().mousePressEvent(event)

    def mouseReleaseEvent(self, event):
        cls = DraggableTabBar
        if event.button() == QtCore.Qt.LeftButton:
            if cls.initializing_drag_:
                if self.parent().indexOf(cls.drag_tab_info_.widget) <= 0:
                    cls.dragging_widget_ = cls.drag_tab_info_.widget
                    cls.dragging_widget_.setParent(None)
                    cls.dragging_widget_.setWindowFlags(
                        QtCore.Qt.FramelessWindowHint)
                else:
                    cls.dragging_widget_ = self.window()
                cls.initializing_drag_ = False
                cls.dragging_widget_.window().raise_()
            else:
                if cls.dragging_widget_:
                    win_rect = cls.dragging_widget_.geometry()
                    win_rect.moveTo(event.globalPos())
                    idx = self.parent().indexOf(cls.drag_tab_info_.widget)
                    if idx >= 0:
                        self.parent().removeTab(idx)
                    self.createWindowRequested.emit(win_rect, cls.drag_tab_info_)
                    self.destroyUnnecessaryWindow()
                cls.dragging_widget_ = None
                cls.drag_tab_info_ = TabInfo()
                self.releaseMouse()
        self.click_point = QtCore.QPoint()
        self.can_start_drag = False
        super().mouseReleaseEvent(event)

    def mouseMoveEvent(self, event):
        cls = DraggableTabBar
        if cls.drag_tab_info_.widget is None:
            return

        if not self.can_start_drag:
            moved_length = (event.pos() - self.click_point).manhattanLength()
            self.can_start_drag = moved_length > QtWidgets.qApp.startDragDistance()

        if cls.dragging_widget_:
            for bar_inst in cls._tabBarInstances():
                bar_region = bar_inst.visibleRegion()
                bar_region.translate(bar_inst.mapToGlobal(QtCore.QPoint(0, 0)))
                if bar_region.contains(event.globalPos()):
                    if (bar_inst == self):
                        self.startTabMove()
                        event.accept()
                        return
                    else:
                        self.releaseMouse()
                        bar_inst.grabMouse()
                        event.accept()
                        return
        widget_rect = self.geometry()
        widget_rect.moveTo(0, 0)
        if widget_rect.contains(event.pos()):
            super().mouseMoveEvent(event)
        elif cls.dragging_widget_ is None and self.can_start_drag:
            # start dragging
            self.startDrag()
            event.accept()
            return

        if cls.dragging_widget_:
            cls.dragging_widget_.move(event.globalPos() + QtCore.QPoint(1, 1))
            cls.dragging_widget_.show()

    def startDrag(self):
        cls = DraggableTabBar
        if self.count() > 1:
            parent = self.parent()
            idx = parent.indexOf(cls.drag_tab_info_.widget)
            parent.removeTab(idx)
            cls.drag_tab_info_.widget.setParent(None)
        cls.dragging_widget_ = None
        cls.initializing_drag_ = True
        release_event = self.createMouseEvent(
            QtCore.QEvent.MouseButtonRelease,
            self.mapFromGlobal(QtGui.QCursor.pos()))
        QtWidgets.QApplication.postEvent(self, release_event)

    def createMouseEvent(self, event_type, pos = QtCore.QPoint()):
        if pos.isNull():
            global_pos = QtGui.QCursor.pos()
        else:
            global_pos = self.mapToGlobal(pos)
        modifiers = QtWidgets.QApplication.keyboardModifiers()

        event = QtGui.QMouseEvent(
            event_type, pos, global_pos,
            QtCore.Qt.LeftButton, QtCore.Qt.LeftButton, modifiers)
        return event

    def startTabMove(self):
        cls = DraggableTabBar
        global_pos = QtGui.QCursor.pos()
        pos = self.mapFromGlobal(global_pos)

        if cls.drag_tab_info_.widget.parent() is not None:
            parent = cls.drag_tab_info_.widget.parent().parent()
            idx = parent.indexOf(cls.drag_tab_info_.widget)
            parent.removeTab(idx)
            parent.window().hide()
        
        idx = self.tabAt(pos)
        self.insertCurrentTabInfo(idx)
        cls.dragging_widget_ = None
        cls.drag_tab_info_ = TabInfo()

        press_event = self.createMouseEvent(
            QtCore.QEvent.MouseButtonPress, self.tabRect(idx).center())
        QtWidgets.QApplication.postEvent(self, press_event)
        self.destroyUnnecessaryWindow()
        self.window().raise_()

    def destroyUnnecessaryWindow(self):
        cls = DraggableTabBar
        for bar_inst in cls._tabBarInstances():
            if bar_inst.count() == 0 \
               and (not bar_inst.isVisible() or bar_inst.parent().parent() is None):
                bar_inst.deleteLater()
                bar_inst.parent().deleteLater()

    def insertCurrentTabInfo(self, idx):
        cls = DraggableTabBar
        parent = self.parent()
        parent.insertTab(
            idx,
            cls.drag_tab_info_.widget,
            cls.drag_tab_info_.icon,
            cls.drag_tab_info_.text
        )
        parent.setTabToolTip(idx, cls.drag_tab_info_.tool_tip)
        parent.setTabWhatsThis(idx, cls.drag_tab_info_.whats_this)
        parent.setCurrentWidget(cls.drag_tab_info_.widget)

    @classmethod
    def _tabBarInstances(cls):
        return [w for w in QtWidgets.qApp.allWidgets() if w.__class__ == cls]

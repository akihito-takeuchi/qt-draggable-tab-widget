# Copyright (c) 2019 Akihito Takeuchi
# Distributed under the MIT License : http://opensource.org/licenses/MIT

from PyQt5 import QtWidgets, QtGui, QtCore

Signal = QtCore.pyqtSignal
Slot = QtCore.pyqtSlot

class TabInfo:
    def __init__(self):
        self.widget = None
        self.text = None
        self.icon = None
        self.tool_tip = None
        self.whats_this = None

class DraggableTabWidget(QtWidgets.QTabWidget):
    tab_widgets = []

    def __init__(self, parent = None):
        super().__init__(parent)
        tab_bar = DraggableTabBar(self)
        self.setTabBar(tab_bar)
        tab_bar.createWindowRequested.connect(self.createNewWindow)
        self.setMovable(True)
        DraggableTabWidget.tab_widgets.append(self)

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

    _drag_tab_info = TabInfo()

    def __init__(self, parent = None):
        super().__init__(parent)
        self.setAcceptDrops(True)
        self.current_widget = None
        self.click_point = QtCore.QPoint()
        QtWidgets.qApp.installEventFilter(self)

    def mousePressEvent(self, event):
        if event.button() == QtCore.Qt.LeftButton:
            current_index = self.tabAt(event.pos())
            self.parentWidget().setCurrentIndex(current_index)
            self.current_widget = self.parentWidget().currentWidget()
            tab_info = TabInfo()
            tab_info.widget = None
            tab_info.text = self.tabText(self.currentIndex())
            tab_info.icon = self.tabIcon(self.currentIndex())
            tab_info.tool_tip = self.tabToolTip(self.currentIndex())
            tab_info.whats_this = self.tabWhatsThis(self.currentIndex())
            DraggableTabBar._drag_tab_info = tab_info
            self.click_point = event.globalPos() - self.window().pos()
        super().mousePressEvent(event)

    def mouseReleaseEvent(self, event):
        self.click_point = QtCore.QPoint()
        DraggableTabBar._drag_tab_info.widget = None
        super().mouseReleaseEvent(event)

    def mouseMoveEvent(self, event):
        if self.click_point.isNull():
            return

        if self.geometry().contains(event.pos()):
            super().mouseMoveEvent(event)
        elif not DraggableTabBar._drag_tab_info.widget:
            # start dragging
            self.startDrag()
            event.accept()
        else:
            super().mouseMoveEvent(event)

    def startDrag(self):
        DraggableTabBar._drag_tab_info.widget = self.current_widget
        idx = self.parentWidget().indexOf(self.current_widget)
        self.parentWidget().removeTab(idx)
        if self.count() == 0:
            self.window().hide()
        drag = QtGui.QDrag(self)
        data = QtCore.QMimeData()
        drag.setMimeData(data)
        drag.setPixmap(self.current_widget.grab())
        drop_action = drag.exec_(QtCore.Qt.MoveAction)
        if drop_action == QtCore.Qt.IgnoreAction:
            if drag.target() is self:
                # Back to normal move event
                self.sendButtonRelease()
                self.startTabMove()
                DraggableTabBar._drag_tab_info.widget = None
            else:
                # Drop out. Create new window and complete mouse move
                global_pos = QtGui.QCursor.pos()
                pos = self.mapFromGlobal(global_pos)
                win_rect = self.parent().geometry()
                win_rect.moveTo(global_pos)
                self.createWindowRequested.emit(
                    win_rect, DraggableTabBar._drag_tab_info)
                DraggableTabBar._drag_tab_info.widget = None
                release_event = QtGui.QMouseEvent(
                    QtCore.QEvent.MouseButtonRelease, pos, global_pos,
                    QtCore.Qt.LeftButton, QtCore.Qt.LeftButton,
                    QtWidgets.QApplication.keyboardModifiers())
                QtWidgets.QApplication.postEvent(self, release_event)
        if self.count() == 0:
            self.window().deleteLater()

    def eventFilter(self, obj, event):
        if obj is self.window().windowHandle():
            return self.dispatchEvent(event)
        return False

    def dispatchEvent(self, event):
        event_func_map = {
            QtCore.QEvent.MouseMove          : self.mouseMoveEvent,
            QtCore.QEvent.MouseButtonRelease : self.mouseReleaseEvent,
        }
        if event.type() not in event_func_map:
            return False

        if event.type() == QtCore.QEvent.MouseMove \
           and DraggableTabBar._drag_tab_info.widget:
            return False

        if event.type() == QtCore.QEvent.MouseButtonRelease \
           and self.click_point.isNull():
            return False
        
        pos = self.mapFromGlobal(event.globalPos())
        new_event = QtGui.QMouseEvent(
            event.type(), pos, event.globalPos(),
            event.button(), event.buttons(), event.modifiers())
        event_func_map[event.type()](new_event)
        return True

    def dragEnterEvent(self, event):
        # When mouse cursor is back to source widget, cancel drag
        if self.geometry().contains(event.pos()) \
           and event.source() is self:
            QtGui.QDrag.cancel()
            event.accept()
            return
        event.acceptProposedAction()
        self.sendButtonRelease(event.source())

    def dropEvent(self, event):
        event.acceptProposedAction()
        self.startTabMove()

    def sendButtonRelease(self, obj = None):
        global_pos = QtGui.QCursor.pos()
        pos = self.mapFromGlobal(global_pos)
        modifiers = QtWidgets.QApplication.keyboardModifiers()

        release_event = QtGui.QMouseEvent(
            QtCore.QEvent.MouseButtonRelease, pos, global_pos,
            QtCore.Qt.LeftButton, QtCore.Qt.LeftButton, modifiers)
        if obj is None:
            obj = self
        QtWidgets.QApplication.postEvent(obj, release_event)

    def startTabMove(self):
        global_pos = QtGui.QCursor.pos()
        pos = self.mapFromGlobal(global_pos)
        modifiers = QtWidgets.QApplication.keyboardModifiers()

        idx = self.tabAt(pos)
        self.parentWidget().insertTab(
            idx,
            DraggableTabBar._drag_tab_info.widget,
            DraggableTabBar._drag_tab_info.icon,
            DraggableTabBar._drag_tab_info.text
        )
        self.parentWidget().setTabToolTip(
            idx, DraggableTabBar._drag_tab_info.tool_tip)
        self.parentWidget().setTabWhatsThis(
            idx, DraggableTabBar._drag_tab_info.whats_this)
        self.parentWidget().setCurrentIndex(idx)

        press_event = QtGui.QMouseEvent(
            QtCore.QEvent.MouseButtonPress, pos, global_pos,
            QtCore.Qt.LeftButton, QtCore.Qt.LeftButton, modifiers)
        QtWidgets.QApplication.postEvent(self, press_event)

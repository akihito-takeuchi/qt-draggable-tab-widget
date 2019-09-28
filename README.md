# Qt Draggable tab widget

This widget supports draggable tab. User can drag the tab to detach it from the tab widget. Python and C++ is supported. The codes were developed in following environment.

* C++
  * Linux (CentOS7) Qt 5.10.1
  * macOS Qt 5.7.1
* Python
  * Linux (CentOS7) Qt 5.10.1 + PyQt5

This documentation uses Python code. C++ is almost the same.

## Usage

This widget can be used as same as QTabWidget.

    w = DraggableTabWidget()
    w.addTab(QtWidgets.QLabel("Widget", "Tab"))
    w.show()

## Default detach tab behavior

When detaching a tab, This widget creates new DraggableTabWidget and adds the detached tab to new widget.

## Customize detach tab behavior

To customize the detach tab behavior, create new class inherits DraggableTabWidget, and override 'createNewWindow' method. The signature of the method (slot) is as follows.

    @Slot(QtCore.QRect, TabInfo)
    def createNewWindow(self, win_rect, tab_info):

The first argument 'win_rect' has the size of source DraggableTabWidget at current mouse cursor. The 'TabInfo' is a class containing tab information.

    class TabInfo:
        def __init__(self):
            self.widget = None
            self.text = None
            self.icon = None
            self.tool_tip = None
            self.whats_this = None

Default implementation of 'createNewWindow' method just creates a new window as below.

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

# Example

Please refer to python/example/test.py

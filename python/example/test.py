import os
import sys
from PyQt5 import QtWidgets

sys.path.append(os.path.join(
    os.path.dirname(os.path.abspath(__file__)), "..", "src"))

from draggabletabwidget import DraggableTabWidget

def create_table():
    tbl = QtWidgets.QTableWidget()
    tbl.setRowCount(5)
    tbl.setColumnCount(5)
    return tbl

def main():
    a = QtWidgets.QApplication(sys.argv)
    w = DraggableTabWidget()
    for i in range(5):
        w.addTab(create_table(), "Tab{0}".format(i + 1))
    w.resize(800, 400)
    w.show()
    return a.exec_()

if __name__ == "__main__":
    exit(main())
    

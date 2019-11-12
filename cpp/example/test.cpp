// Copyright (c) 2019 Akihito Takeuchi
// Distributed under the MIT License : http://opensource.org/licenses/MIT

#include "draggabletabwidget.h"

#include <QApplication>
#include <QTableWidget>

QTableWidget* CreateTable() {
  auto tbl = new QTableWidget;
  tbl->setRowCount(5);
  tbl->setColumnCount(5);
  return tbl;
}

int main(int argc, char* argv[]) {
  QApplication a(argc, argv);
  DraggableTabWidget w;
  for (int i = 1; i <= 5; ++ i)
    w.addTab(CreateTable(),
             QString("Tab%1").arg(i));
  w.resize(600, 400);
  w.show();
  return a.exec();
}

// Copyright (c) 2019 Akihito Takeuchi
// Distributed under the MIT License : http://opensource.org/licenses/MIT

#include "draggabletabwidget.h"

#include <QApplication>
#include <QLabel>

int main(int argc, char* argv[]) {
  QApplication a(argc, argv);
  DraggableTabWidget w;
  for (int i = 1; i <= 5; ++ i)
    w.addTab(new QLabel(QString("Widget %1").arg(i)),
             QString("Tab%1").arg(i));
  w.resize(600, 400);
  w.show();
  return a.exec();
}

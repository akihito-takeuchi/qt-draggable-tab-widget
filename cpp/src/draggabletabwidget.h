// Copyright (c) 2019 Akihito Takeuchi
// Distributed under the MIT License : http://opensource.org/licenses/MIT

#pragma once

#include <QTabWidget>
#include <QIcon>

class DraggableTabWidget : public QTabWidget {
  Q_OBJECT
 public:
  class TabInfo {
   public:
    TabInfo() = default;
    TabInfo(QWidget* widget, const QString& text, const QIcon& icon,
            const QString& tool_tip, const QString& whats_this)
        : widget_(widget), text_(text), icon_(icon),
          tool_tip_(tool_tip), whats_this_(whats_this) {}
    ~TabInfo() = default;

    QWidget* widget() const { return widget_; }
    QString text() const { return text_; }
    QIcon icon() const { return icon_; }
    QString toolTip() const { return tool_tip_; }
    QString whatsThis() const { return whats_this_; }

   private:
    QWidget* widget_ = nullptr;
    QString text_;
    QIcon icon_;
    QString tool_tip_;
    QString whats_this_;
  };

  DraggableTabWidget(QWidget* parent = nullptr);
  ~DraggableTabWidget();

 public slots:
  virtual QWidget* createNewWindow(const QRect& win_rect,
                                   const TabInfo& tab_info);
};

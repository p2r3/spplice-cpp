#ifndef TOOLS_QT_H
#define TOOLS_QT_H

#include <QWidget>
#include <QPixmap>
#include <QVBoxLayout>
#include "package.h" // ToolsPackage

class ToolsQT {
  public:
    static QPixmap getRoundedPixmap (const QPixmap &src, int radius);
    static void displayErrorPopup (const std::string &title, const std::string &message);
};

#endif

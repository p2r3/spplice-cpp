#ifndef TOOLS_QT_H
#define TOOLS_QT_H

#include <QWidget>
#include <QPixmap>
#include "package.h" // ToolsPackage

class ToolsQT {
  public:
    static QPixmap getRoundedPixmap (const QPixmap &src, int radius);
    static QWidget *createPackageItem (ToolsPackage::PackageData *package);
};

#endif

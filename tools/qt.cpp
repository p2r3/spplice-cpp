#include <iostream>
#include <QApplication>
#include <QPixmap>
#include <QPainter>
#include <QPainterPath>
#include <filesystem>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <functional>
#include "../ui/errordialog.h"
#include "curl.h"    // ToolsCURL
#include "qt.h"      // ToolsQT
#include "install.h" // ToolsInstall
#include "package.h" // ToolsPackage

// Definitions for this source file
#include "qt.h"

// Given a path to an image and a QSize, returns a pixmap of that image
QPixmap ToolsQT::getPixmapFromPath (const std::filesystem::path &path, const QSize size) {

#ifndef TARGET_WINDOWS
  return QPixmap(QString::fromStdString(path.string())).scaled(size, Qt::IgnoreAspectRatio, Qt::SmoothTransformation);
#else
  return QPixmap(QString::fromStdWString(path.wstring())).scaled(size, Qt::IgnoreAspectRatio, Qt::SmoothTransformation);
#endif

}

// Returns a version of the input pixmap with rounded corners
QPixmap ToolsQT::getRoundedPixmap (const QPixmap &src, int radius) {

  // Create a new QPixmap with the same size as the source pixmap
  QPixmap rounded(src.size());
  rounded.fill(Qt::transparent); // Fill with transparency

  // Set up QPainter to draw on the rounded pixmap
  QPainter painter(&rounded);
  painter.setRenderHint(QPainter::Antialiasing);
  painter.setRenderHint(QPainter::SmoothPixmapTransform);

  // Create a path for the rounded corners
  QPainterPath path;
  path.addRoundedRect(src.rect(), radius, radius);

  // Clip the painter to the rounded rectangle
  painter.setClipPath(path);

  // Draw the original pixmap onto the rounded pixmap
  painter.drawPixmap(0, 0, src);

  return rounded;

}

void ToolsQT::displayErrorPopup (const std::string title, const std::string message) {

  QDialog *dialog = new QDialog;
  Ui::ErrorDialog dialogUI;
  dialogUI.setupUi(dialog);

  dialogUI.ErrorTitle->setText(QString::fromStdString(title));
  dialogUI.ErrorText->setText(QString::fromStdString(message));
  dialog->exec();

}

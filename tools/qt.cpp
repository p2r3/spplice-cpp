#include <iostream>
#include <QApplication>
#include <QPixmap>
#include <QPainter>
#include <QPainterPath>
#include <filesystem>
#include <thread>
#include "../ui/packageitem.h"
#include "curl.h"    // ToolsCURL
#include "qt.h"      // ToolsQT
#include "install.h" // ToolsInstall
#include "package.h" // ToolsPackage
#include "../globals.h"

// Definitions for this source file
#include "qt.h"

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

// Finds a widget by its name. This is required as threads and lambdas make widget references unreliable.
QWidget *ToolsQT::findByName (const std::string &widgetName) {

  QString widgetQName = QString::fromStdString(widgetName);
  QWidget *output;

  // Notice that we don't immediately return the result once we find it.
  // I'm not sure why, but it seems that if you need a child of the element you're searching for,
  // you run into segmentation faults unless you've iterated up to the child element as well.
  foreach (QWidget *widget, QApplication::allWidgets()) {
    if (widget->objectName() != widgetQName) continue;
    output = widget;
  }

  return output;

}

// Generates a PackageItem instance of the given package for rendering
QWidget *ToolsQT::createPackageItem (ToolsPackage::PackageData *package) {

  QWidget *item = new QWidget;
  Ui::PackageItem itemUI;
  itemUI.setupUi(item);

  std::string packageName = package->name;
  item->setObjectName(QString::fromStdString(packageName));

  // Set values of text-based elements
  itemUI.PackageTitle->setText(QString::fromStdString(package->title));
  itemUI.PackageDescription->setText(QString::fromStdString(package->description));

  // Connect the install button
  std::string fileURL = package->file;
  QWidget::connect(itemUI.PackageInstallButton, &QPushButton::clicked, [fileURL, packageName]() {

    if (BUSY_INSTALLING) {
      std::cerr << "Cannot install package while another package is busy!" << std::endl;
      return;
    }
    BUSY_INSTALLING = true;

    std::thread installThread([fileURL, packageName]() {

      QPushButton *installButton = ToolsQT::findByName(packageName)->findChild<QPushButton *>("PackageInstallButton");
      installButton->setText("Installing...");
      installButton->setStyleSheet("color: #faa81a;");

      ToolsInstall::installRemoteFile(fileURL, [](const std::string message) {
        std::cerr << "Error: " << message << std::endl;
      }, [packageName]() {
        QPushButton *installButton = ToolsQT::findByName(packageName)->findChild<QPushButton *>("PackageInstallButton");
        installButton->setText("Installed");
      });

      BUSY_INSTALLING = false;
      installButton->setText("Install");
      installButton->setStyleSheet("");

    });
    installThread.detach();

  });

  // Prepare for package icon download
  std::string imagePath = TEMP_DIR / (package->name + "_icon");
  std::string imageURL = package->icon;

  // Prepare the package icon on a separate thread
  std::thread iconThread([imagePath, imageURL, packageName]() {

    // Check if we already have a cached package icon
    if (!std::filesystem::exists(imagePath)) {
      // Attempt the download 5 times before giving up
      for (int attempts = 0; attempts < 5; attempts ++) {
        if (ToolsCURL::downloadFile(imageURL, imagePath)) {
          break;
        }
      }
    }

    QLabel *packageIcon = ToolsQT::findByName(packageName)->findChild<QLabel *>("PackageIcon");

    // Create a pixmap for the icon
    QSize labelSize = packageIcon->size();
    QPixmap iconPixmap = QPixmap(QString::fromStdString(imagePath)).scaled(labelSize, Qt::IgnoreAspectRatio, Qt::SmoothTransformation);
    QPixmap iconRoundedPixmap = ToolsQT::getRoundedPixmap(iconPixmap, 10);

    // Set the package icon
    packageIcon->setPixmap(iconRoundedPixmap);

  });
  iconThread.detach();

  return item;
}

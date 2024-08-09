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

// Generates a PackageItem instance of the given package for rendering
QWidget *ToolsQT::createPackageItem (ToolsPackage::PackageData *package) {

  QWidget *item = new QWidget;
  Ui::PackageItem itemUI;
  itemUI.setupUi(item);

  // Set values of text-based elements
  itemUI.PackageTitle->setText(QString::fromStdString(package->title));
  itemUI.PackageDescription->setText(QString::fromStdString(package->description));

  // Connect the install button
  std::string fileURL = package->file;
  QWidget::connect(itemUI.PackageInstallButton, &QPushButton::clicked, [fileURL]() {
    ToolsInstall::installRemoteFile(fileURL);
  });

  // Prepare for package icon download
  std::string imagePath = TEMP_DIR / (package->name + "_icon");
  std::string imageURL = package->icon;

  // Prepare the package icon on a separate thread
  std::thread iconThread([imagePath, imageURL, itemUI]() {

    // Check if we already have a cached package icon
    if (!std::filesystem::exists(imagePath)) {
      // Attempt the download 5 times before giving up
      for (int attempts = 0; attempts < 5; attempts ++) {
        if (ToolsCURL::downloadFile(imageURL, imagePath)) {
          break;
        }
      }
    }

    // Create a pixmap for the icon
    QSize labelSize = itemUI.PackageIcon->size();
    QPixmap iconPixmap = QPixmap(QString::fromStdString(imagePath)).scaled(labelSize, Qt::IgnoreAspectRatio, Qt::SmoothTransformation);
    QPixmap iconRoundedPixmap = ToolsQT::getRoundedPixmap(iconPixmap, 10);

    // Set the package icon
    itemUI.PackageIcon->setPixmap(iconRoundedPixmap);

  });
  iconThread.detach();

  return item;

}

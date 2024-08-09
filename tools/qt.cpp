#include <iostream>
#include <QApplication>
#include <QPixmap>
#include <QPainter>
#include <QPainterPath>
#include <filesystem>
#include <thread>
#include "../ui/packageitem.h"
#include "../ui/errordialog.h"
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
#define TOOLS_QT_FIND_BY_NAME(widgetName, result) { \
  QString widgetQName = QString::fromStdString(widgetName); \
  result = nullptr; \
  foreach (QWidget *widget, QApplication::allWidgets()) { \
    if (widget->objectName() != widgetQName) continue; \
    result = widget; \
    break; \
  } \
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

    // If a package is already installing (or installed), exit early
    if (BUSY_INSTALLING) {
      QDialog *dialog = new QDialog;
      Ui::ErrorDialog dialogUI;
      dialogUI.setupUi(dialog);

      dialogUI.ErrorTitle->setText("Spplice is busy");
      dialogUI.ErrorText->setText("You cannot install two packages at once!");
      dialog->exec();

      return;
    }
    BUSY_INSTALLING = true;

    std::thread installThread([fileURL, packageName]() {

      QWidget *packageItem;
      TOOLS_QT_FIND_BY_NAME(packageName, packageItem);
      QPushButton *installButton = packageItem->findChild<QPushButton *>("PackageInstallButton");

      installButton->setText("Installing...");
      installButton->setStyleSheet("color: #faa81a;");

      // Download the package's .tar.xz file into its temporary path
      ToolsCURL::downloadFile(fileURL, TEMP_DIR / "current_package");

      // Install the file
      ToolsInstall::installTempFile([](const std::string message) {

        // If unsuccessful, show the error dialog
        QDialog *dialog = new QDialog;
        Ui::ErrorDialog dialogUI;
        dialogUI.setupUi(dialog);

        dialogUI.ErrorTitle->setText("Installation aborted");
        dialogUI.ErrorText->setText(QString::fromStdString(message));
        dialog->exec();

      }, [packageName]() {

        // If successful, set the button text to "Installed"
        QWidget *packageItem;
        TOOLS_QT_FIND_BY_NAME(packageName, packageItem);
        QPushButton *installButton = packageItem->findChild<QPushButton *>("PackageInstallButton");

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

    QWidget *packageItem;
    TOOLS_QT_FIND_BY_NAME(packageName, packageItem);
    QLabel *packageIcon = packageItem->findChild<QLabel *>("PackageIcon");

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

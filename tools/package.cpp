#include <string>
#include "rapidjson/document.h"
#include "../ui/packageitem.h"
#include "curl.h" // ToolsCURL
#include "qt.h" // ToolsQT
#include "install.h" // ToolsInstall
#include "../globals.h"

// Definitions for this source file
#include "package.h"

// Constructs the PackageData instance from a JSON object
ToolsPackage::PackageData::PackageData (rapidjson::Value &package) {
  this->name = package["name"].GetString();
  this->title = package["title"].GetString();
  this->author = package["author"].GetString();
  this->file = package["file"].GetString();
  this->icon = package["icon"].GetString();
  this->description = package["description"].GetString();
  this->weight = package["weight"].GetInt();
}

// Generates a PackageItem instance of the given package for rendering
QWidget *ToolsPackage::PackageData::createPackageItem () {

  QWidget *item = new QWidget;
  Ui::PackageItem itemUI;
  itemUI.setupUi(item);

  // Set values of text-based elements
  itemUI.PackageTitle->setText(QString::fromStdString(this->title));
  itemUI.PackageDescription->setText(QString::fromStdString(this->description));

  // Download the package icon
  QString imagePath = QString::fromStdString(TEMP_DIR / (this->name + "_icon"));
  ToolsCURL::downloadFile(this->icon, imagePath.toStdString());

  // Create a pixmap for the icon
  QSize labelSize = itemUI.PackageIcon->size();
  QPixmap iconPixmap = QPixmap(imagePath).scaled(labelSize, Qt::IgnoreAspectRatio, Qt::SmoothTransformation);
  QPixmap iconRoundedPixmap = ToolsQT::getRoundedPixmap(iconPixmap, 10);

  // Set the package icon
  itemUI.PackageIcon->setPixmap(iconRoundedPixmap);

  // Connect the install button
  std::string fileURL = this->file;
  QWidget::connect(itemUI.PackageInstallButton, &QPushButton::clicked, [fileURL]()
                  { ToolsInstall::installRemoteFile(fileURL); });

  return item;

}

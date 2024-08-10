#include <iostream>
#include <string>
#include <QPixmap>
#include "rapidjson/document.h"

#include "../globals.h"
#include "curl.h" // ToolsCURL
#include "qt.h" // ToolsQT

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

void PackageItemWorker::getPackageIcon (const std::string &imageURL, const std::string &imagePath, const QSize iconSize) {

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
  QPixmap iconPixmap = QPixmap(QString::fromStdString(imagePath)).scaled(iconSize, Qt::IgnoreAspectRatio, Qt::SmoothTransformation);
  QPixmap iconRoundedPixmap = ToolsQT::getRoundedPixmap(iconPixmap, 10);

  // Set the package icon
  emit packageIconResult(iconRoundedPixmap);
  emit packageIconReady();

};

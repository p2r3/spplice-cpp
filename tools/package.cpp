#include <iostream>
#include <utility>
#include <thread>
#include <chrono>
#include <string>
#include <QPixmap>
#include "rapidjson/document.h"

#include "../globals.h"
#include "curl.h" // ToolsCURL
#include "qt.h" // ToolsQT
#include "install.h" // ToolsInstall

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

void PackageItemWorker::installPackage (const std::string &fileURL) {

  // If a package is already installing (or installed), exit early
  if (SPPLICE_INSTALL_STATE != 0) {
    ToolsQT::displayErrorPopup("Spplice is busy", "You cannot install two packages at once!");
    return;
  }

  SPPLICE_INSTALL_STATE = 1;
  emit installStateUpdate();

  // Download the package's .tar.xz file into its temporary path
  ToolsCURL::downloadFile(fileURL, TEMP_DIR / "current_package");

  // Attempt installation
  std::pair<bool, std::string> installationResult = ToolsInstall::installTempFile();

  // If installation failed, display error and exit early
  if (installationResult.first == false) {
    ToolsQT::displayErrorPopup("Installation aborted", installationResult.second);

    SPPLICE_INSTALL_STATE = 0;
    emit installStateUpdate();

    return;
  }

  SPPLICE_INSTALL_STATE = 2;
  emit installStateUpdate();

  // Stall until Portal 2 has been closed
  while (ToolsInstall::getProcessPath("portal2_linux") != "") {
    std::this_thread::sleep_for(std::chrono::milliseconds(200));
  }

  // Uninstall the package, reset the state
  ToolsInstall::Uninstall(installationResult.second);

  SPPLICE_INSTALL_STATE = 0;
  emit installStateUpdate();

}

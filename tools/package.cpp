#include <iostream>
#include <fstream>
#include <utility>
#include <thread>
#include <chrono>
#include <string>
#include <functional>
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
  this->title = package["title"].GetString();
  this->author = package["author"].GetString();
  this->description = package["description"].GetString();
  this->version = package["version"].GetString();
  this->file = package["file"].GetString();
  this->icon = package["icon"].GetString();
  rapidjson::Value &args = package["args"];
  for (rapidjson::Value::ConstValueIterator itr = args.Begin(); itr != args.End(); ++itr) {
    this->args.push_back(itr->GetString());
  }
}

void PackageItemWorker::getPackageIcon (ToolsPackage::PackageData package, const QSize iconSize) {

  // Generate a hash from the icon URL to use as a file name
  size_t imageURLHash = std::hash<std::string>{}(package.icon);
  std::filesystem::path imagePath = TEMP_DIR / std::to_string(imageURLHash);

  // Attempt the download 5 times before giving up
  for (int attempts = 0; attempts < 5; attempts ++) {
    if (ToolsCURL::downloadFile(package.icon, imagePath)) break;
  }

  // Create a pixmap for the icon
#ifndef TARGET_WINDOWS
  QPixmap iconPixmap = QPixmap(QString::fromStdString(imagePath.string())).scaled(iconSize, Qt::IgnoreAspectRatio, Qt::SmoothTransformation);
#else
  QPixmap iconPixmap = QPixmap(QString::fromStdWString(imagePath.wstring())).scaled(iconSize, Qt::IgnoreAspectRatio, Qt::SmoothTransformation);
#endif
  QPixmap iconRoundedPixmap = ToolsQT::getRoundedPixmap(iconPixmap, 10);

  // Set the package icon
  emit packageIconResult(iconRoundedPixmap);
  emit packageIconReady();

};

void PackageItemWorker::installPackage (ToolsPackage::PackageData package) {

  // If a package is already installing (or installed), exit early
  if (SPPLICE_INSTALL_STATE != 0) {
    ToolsQT::displayErrorPopup("Spplice is busy", "You cannot install two packages at once!");
    return;
  }

  SPPLICE_INSTALL_STATE = 1;
  emit installStateUpdate();

  // Generate a hash from the file's URL to use as a file name
  size_t fileURLHash = std::hash<std::string>{}(package.file);
  const std::filesystem::path filePath = TEMP_DIR / std::to_string(fileURLHash);

  // Check if we have an up-to-date cache of the package file
  bool cacheFound = false;
  if (std::filesystem::exists(filePath)) {

    std::ifstream versionFile(filePath.string() + ".ver");
    std::string localVersion = "";

    if (versionFile.is_open()) {
      std::getline(versionFile, localVersion);
      versionFile.close();
    }

    if (localVersion == package.version) {
      cacheFound = true;
      std::cout << "Cached package found, skipping download" << std::endl;
    }

  }

  // If the cache was outdated (or not present), download the package file
  if (!cacheFound) {

    if (!ToolsCURL::downloadFile(package.file, filePath)) {
      ToolsQT::displayErrorPopup("Installation aborted", "Failed to download package file.");
      return;
    }

    // Update the version file
    std::ofstream versionFile(filePath.string() + ".ver");
    if (versionFile.is_open()) {
      versionFile << package.version;
      versionFile.close();
    } else {
      std::cout << "Couldn't open package version file for writing" << std::endl;
    }

  }

    // Attempt installation
#ifndef TARGET_WINDOWS
  std::pair<bool, std::string> installationResult = ToolsInstall::installPackageFile(filePath);
#else
  std::pair<bool, std::wstring> installationResult = ToolsInstall::installPackageFile(filePath);
#endif

  // If installation failed, display error and exit early
  if (installationResult.first == false) {
#ifndef TARGET_WINDOWS
    ToolsQT::displayErrorPopup("Installation aborted", installationResult.second);
#else
    ToolsQT::displayErrorPopup("Installation aborted", std::string(installationResult.second.begin(), installationResult.second.end()));
#endif

    SPPLICE_INSTALL_STATE = 0;
    emit installStateUpdate();

    std::filesystem::remove(filePath);

    return;
  }

  SPPLICE_INSTALL_STATE = 2;
  emit installStateUpdate();

  // Stall until Portal 2 has been closed
#ifndef TARGET_WINDOWS
  while (ToolsInstall::getProcessPath("portal2_linux") != "") {
#else
  while (ToolsInstall::getProcessPath("portal2.exe") != L"") {
#endif
    std::this_thread::sleep_for(std::chrono::milliseconds(200));
  }

  // Uninstall the package, reset the state
  ToolsInstall::Uninstall(installationResult.second);

  SPPLICE_INSTALL_STATE = 0;
  emit installStateUpdate();

}

#include <iostream>
#include <fstream>
#include <utility>
#include <thread>
#include <chrono>
#include <string>
#include <functional>
#include <QPixmap>
#include <QJsonObject>
#include <QJsonArray>
#include <QJsonValue>

#include "../globals.h"
#include "curl.h" // ToolsCURL
#include "qt.h" // ToolsQT
#include "install.h" // ToolsInstall

// Definitions for this source file
#include "package.h"

// Constructs the PackageData instance from a JSON object
ToolsPackage::PackageData::PackageData (QJsonObject package) {
  this->title = package["title"].toString().toStdString();
  this->author = package["author"].toString().toStdString();
  this->description = package["description"].toString().toStdString();
  this->version = package["version"].toString().toStdString();
  this->file = package["file"].toString().toStdString();
  this->icon = package["icon"].toString().toStdString();
  QJsonArray args = package["args"].toArray();
  for (const QJsonValue &arg : args) {
    this->args.push_back(arg.toString().toStdString());
  }
}

// Checks if the given file exists and is up-to-date
bool validateFileVersion (std::filesystem::path filePath, const std::string &version) {

  // Check if the given file exists
  if (!std::filesystem::exists(filePath)) return false;

  // Check if the respective version file exists
  filePath += ".ver";
  if (!std::filesystem::exists(filePath)) return false;

  // Read the version file
  std::ifstream versionFile(filePath);
  std::string localVersion = "";

  if (versionFile.is_open()) {
    std::getline(versionFile, localVersion);
    versionFile.close();
  }

  return localVersion == version;

}

// Write a version file for the given file
bool updateFileVersion (std::filesystem::path filePath, const std::string &version) {

  filePath += ".ver";
  std::ofstream versionFile(filePath);

  if (versionFile.is_open()) {
    versionFile << version;
    versionFile.close();
    return true;
  }
  return false;

}

void PackageItemWorker::getPackageIcon (const ToolsPackage::PackageData *package, const QSize iconSize) {

  // Generate a hash from the icon URL to use as a file name
  size_t imageURLHash = std::hash<std::string>{}(package->icon);
  std::filesystem::path imagePath = TEMP_DIR / std::to_string(imageURLHash);

  // Check if we have a valid icon cache
  if (!validateFileVersion(imagePath, package->version)) {
    updateFileVersion(imagePath, package->version);
    // Attempt the download 5 times before giving up
    for (int attempts = 0; attempts < 5; attempts ++) {
      if (ToolsCURL::downloadFile(package->icon, imagePath)) break;
    }
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

void PackageItemWorker::installPackage (const ToolsPackage::PackageData *package) {

  // If a package is already installing (or installed), exit early
  if (SPPLICE_INSTALL_STATE != 0) {
    ToolsQT::displayErrorPopup("Spplice is busy", "You cannot install two packages at once!");
    return;
  }

  SPPLICE_INSTALL_STATE = 1;
  emit installStateUpdate();

  // Generate a hash from the file's URL to use as a file name
  size_t fileURLHash = std::hash<std::string>{}(package->file);
  const std::filesystem::path filePath = TEMP_DIR / std::to_string(fileURLHash);

  // Download the package file if we don't have a valid cache
  if (validateFileVersion(filePath, package->version)) {
    std::cout << "Cached package found, skipping download" << std::endl;
  } else {
    if (!updateFileVersion(filePath, package->version)) {
      std::cout << "Couldn't open package version file for writing" << std::endl;
    }
    if (!ToolsCURL::downloadFile(package->file, filePath)) {
      ToolsQT::displayErrorPopup("Installation aborted", "Failed to download package file.");
      return;
    }
  }

    // Attempt installation
#ifndef TARGET_WINDOWS
  std::pair<bool, std::string> installationResult = ToolsInstall::installPackageFile(filePath, package->args);
#else
  std::pair<bool, std::wstring> installationResult = ToolsInstall::installPackageFile(filePath, package->args);
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

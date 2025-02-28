#include <iostream>
#include <fstream>
#include <utility>
#include <thread>
#include <chrono>
#include <string>
#include <functional>
#include <QPixmap>
#include <QWidget>
#include <QThread>
#include <QObject>
#include <QDialog>
#include <QJsonObject>
#include <QJsonArray>
#include <QJsonValue>

#include "../ui/packageitem.h"
#include "../ui/packageinfo.h"

#include "../globals.h"
#include "curl.h" // ToolsCURL
#include "qt.h" // ToolsQT
#include "install.h" // ToolsInstall

// Definitions for this source file
#include "package.h"

// Constructs the PackageData instance from a JSON object
ToolsPackage::PackageData::PackageData (QJsonObject package, const std::string &repoURL) {

  // Keep track of the repository which this package came from
  this->repository = repoURL;

  // These properties should be available even in old repositories
  this->title = package["title"].toString().toStdString();
  this->author = package["author"].toString().toStdString();
  this->file = package["file"].toString().toStdString();
  this->icon = package["icon"].toString().toStdString();

  // Convert plaintext newlines to <br> tags
  std::string rawDescription = package["description"].toString().toStdString();
  this->description.reserve(rawDescription.size());

  for (char c : rawDescription) {
    if (c == '\n') this->description += "<br/>";
    else this->description += c;
  }

  // The version and args properties were introduced in version 3
  // Use a placeholder for version if not provided
  if (!package.contains("version")) this->version = "1.0.0";
  else this->version = package["version"].toString().toStdString();

  // Leave args blank if not provided
  if (package.contains("args")) {
    QJsonArray args = package["args"].toArray();
    for (const QJsonValue &arg : args) {
      this->args.push_back(arg.toString().toStdString());
    }
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
  std::filesystem::path imagePath = CACHE_DIR / std::to_string(imageURLHash);

  // Check if we have a valid icon cache
  if (!validateFileVersion(imagePath, package->version)) {
    updateFileVersion(imagePath, package->version);
    // Attempt the download 5 times before giving up
    for (int attempts = 0; attempts < 5; attempts ++) {
      if (ToolsCURL::downloadFile(package->icon, imagePath)) break;
    }
  }

  // Create a pixmap for the icon
  QPixmap iconPixmap = ToolsQT::getPixmapFromPath(imagePath, iconSize);
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

  // Store the package archive path, assigned in branch below
  std::filesystem::path filePath;

  // Avoid downloading packages from the special "local" repository
  if (package->repository == "local") {
    // Point directly to the local package's archive file
    filePath = (APP_DIR / "local") / package->file;
  } else {
    // Generate a hash from the file's URL to use as a file name
    size_t fileURLHash = std::hash<std::string>{}(package->file);
    std::filesystem::path filePath = CACHE_DIR / std::to_string(fileURLHash);

    // Download the package file if we don't have a valid cache
    if (CACHE_ENABLE && validateFileVersion(filePath, package->version)) {
      LOGFILE << "[I] Cached package found, skipping download" << std::endl;
    } else {
      if (CACHE_ENABLE && !updateFileVersion(filePath, package->version)) {
        LOGFILE << "[W] Couldn't open package version file for writing" << std::endl;
      }
      if (!ToolsCURL::downloadFile(package->file, filePath)) {
        ToolsQT::displayErrorPopup("Installation aborted", "Failed to download package file.");
        return;
      }
    }
  }


  // Attempt installation
  std::string installationResult = ToolsInstall::installPackageFile(filePath, package->args);

  // Remove downloaded archive post-installation if caching is disabled
  if (!CACHE_ENABLE) std::filesystem::remove(filePath);

  // If installation failed, display error and exit early
  if (installationResult != "") {
    ToolsQT::displayErrorPopup("Installation aborted", installationResult);

    SPPLICE_INSTALL_STATE = 0;
    emit installStateUpdate();

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
  ToolsInstall::uninstall();

  SPPLICE_INSTALL_STATE = 0;
  emit installStateUpdate();

}

QWidget* ToolsPackage::createPackageItem (const ToolsPackage::PackageData *package) {

  // Create the package item widget
  QWidget *item = new QWidget;
  Ui::PackageItem itemUI;
  itemUI.setupUi(item);

  // Set the title and description
  itemUI.PackageTitle->setText(QString::fromStdString(package->title));
  itemUI.PackageDescription->setText(QString::fromStdString(package->description));

  // Tag the item with the package's repository
  item->setProperty("packageRepository", QString::fromStdString(package->repository));

  // Connect the install button
  QPushButton *installButton = itemUI.PackageInstallButton;
  QObject::connect(installButton, &QPushButton::clicked, [installButton, package]() {

    // Create a thread for asynchronous installation
    PackageItemWorker *worker = new PackageItemWorker;
    QThread *workerThread = new QThread;
    worker->moveToThread(workerThread);

    // Connect the task of installing the package to the worker
    QObject::connect(workerThread, &QThread::started, worker, [worker, package]() {
      QMetaObject::invokeMethod(worker, "installPackage", Q_ARG(const ToolsPackage::PackageData*, package));
    });

    // Update the button text based on the installation state
    QObject::connect(worker, &PackageItemWorker::installStateUpdate, installButton, [installButton]() {
      switch (SPPLICE_INSTALL_STATE) {
        case 0:
          installButton->setText("Install");
          installButton->setStyleSheet("");
          break;
        case 1:
          installButton->setText("Installing...");
          installButton->setStyleSheet("color: #faa81a;");
          break;
        case 2:
          installButton->setText("Installed");
          installButton->setStyleSheet("color: #faa81a;");
          break;
      }
    });

    // Clean up the thread once it's done
    QObject::connect(worker, &PackageItemWorker::packageIconReady, workerThread, &QThread::quit);
    QObject::connect(worker, &PackageItemWorker::packageIconReady, worker, &PackageItemWorker::deleteLater);
    QObject::connect(workerThread, &QThread::finished, workerThread, &QThread::deleteLater);

    // Start the worker thread
    workerThread->start();

  });

  // Start a new worker thread for asynchronous icon fetching
  PackageItemWorker *worker = new PackageItemWorker;
  QThread *workerThread = new QThread;
  worker->moveToThread(workerThread);

  // Connect the task of fetching the icon to the worker
  QSize iconSize = itemUI.PackageIcon->size();
  QObject::connect(workerThread, &QThread::started, worker, [worker, package, iconSize]() {
    QMetaObject::invokeMethod(worker, "getPackageIcon", Q_ARG(const ToolsPackage::PackageData*, package), Q_ARG(QSize, iconSize));
  });
  QObject::connect(worker, &PackageItemWorker::packageIconResult, itemUI.PackageIcon, &QLabel::setPixmap);

  // Clean up the thread once it's done
  QObject::connect(worker, &PackageItemWorker::packageIconReady, workerThread, &QThread::quit);
  QObject::connect(worker, &PackageItemWorker::packageIconReady, worker, &PackageItemWorker::deleteLater);
  QObject::connect(workerThread, &QThread::finished, workerThread, &QThread::deleteLater);

  // Start the worker thread
  workerThread->start();

  // Connect the "Read more" button
  QObject::connect(itemUI.PackageInfoButton, &QPushButton::clicked, [package]() {

    QDialog *dialog = new QDialog;
    Ui::PackageInfo dialogUI;
    dialogUI.setupUi(dialog);

    // Set text data (title, author, description)
    dialogUI.PackageTitle->setText(QString::fromStdString(package->title));
    dialogUI.PackageAuthor->setText(QString::fromStdString("By " + package->author));
    dialogUI.PackageDescription->setText(QString::fromStdString(package->description));

    // Set the icon - assume the image has already been downloaded
    size_t imageURLHash = std::hash<std::string>{}(package->icon);
    std::filesystem::path imagePath = CACHE_DIR / std::to_string(imageURLHash);

    QSize iconSize = dialogUI.PackageIcon->size();
    QPixmap iconPixmap = ToolsQT::getPixmapFromPath(imagePath, iconSize);
    QPixmap iconRoundedPixmap = ToolsQT::getRoundedPixmap(iconPixmap, 10);
    dialogUI.PackageIcon->setPixmap(iconRoundedPixmap);

    dialog->setWindowTitle(QString::fromStdString("Details for " + package->title));
    dialog->open();

  });

  return item;

}

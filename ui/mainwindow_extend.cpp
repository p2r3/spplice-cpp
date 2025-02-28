// Headers for this source file
#include "mainwindow_extend.h"
// The UI class we're extending
#include "mainwindow.h"

#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <filesystem>
#include <chrono>

#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QJsonValue>
#include <QMessageBox>
// Used for returning UI elements
#include <QVBoxLayout>
#include <QPushButton>

#include "../globals.h" // Project globals
#include "../tools/package.h" // ToolsPackage
#include "../tools/install.h" // ToolsInstall

MainWindow::MainWindow(QWidget *parent) : QMainWindow(parent), ui(new Ui::MainWindow) {
  ui->setupUi(this);
  setAcceptDrops(true);
}

MainWindow::~MainWindow() {
  delete ui;
}

// Accepts drag-and-drop only for events with file URLs
void MainWindow::dragEnterEvent (QDragEnterEvent *event) {
  if (event->mimeData()->hasUrls()) event->acceptProposedAction();
  else event->ignore();
}

// Handles a Spplice package file being dropped into the main window
void MainWindow::dropEvent (QDropEvent *event) {

  // Check the MIME data of the event again
  const QMimeData *mimeData = event->mimeData();
  if (!mimeData->hasUrls()) return event->ignore();

  const std::filesystem::path archivePath = APP_DIR / "local";
  const std::filesystem::path indexPath = APP_DIR / "local.json";
  const std::filesystem::path extractPath = CACHE_DIR / "extracted_package";

  // Ensure that the local repository archive file directory exists
  std::filesystem::create_directories(archivePath);

  // Create a local repository index if one doesn't exist
  if (!std::filesystem::exists(indexPath)) {
    std::ofstream indexFileW(indexPath);
    if (!indexFileW) {
      LOGFILE << "[E] Failed to open local repository file " << indexPath << " for writing." << std::endl;
      QMessageBox::critical(nullptr, "Package Error", "Failed to create local repository file.");
      return;
    }
    // Write a barebones repository index object
    indexFileW << "{\"packages\":[]}";
    indexFileW.flush();
    indexFileW.close();
  }

  // Attempt to open the local repository index file for reading
  std::ifstream indexFileR(indexPath);
  if (!indexFileR.is_open()) {
    LOGFILE << "[E] Failed to open local repository file " << indexPath << " for reading." << std::endl;
    QMessageBox::critical(nullptr, "Package Error", "Failed to read local repository file.");
    return;
  }

  QJsonDocument indexDocument;
  QJsonObject indexObject;
  QJsonArray packageArray;

  try {
    // Read the contents of the index file into a string
    std::stringstream buffer;
    buffer << indexFileR.rdbuf();
    // Parse the JSON string to get the package array
    indexDocument = QJsonDocument::fromJson(QString::fromStdString(buffer.str()).toUtf8());
    indexObject = indexDocument.object();
    packageArray = indexObject["packages"].toArray();
  } catch (const std::exception &e) {
    QMessageBox::critical(nullptr, "Package Error", "Failed to parse local package repository.");
    std::filesystem::remove_all(extractPath);
    return;
  }

  // Iterate over all URLs (files) dropped
  foreach (const QUrl &url, mimeData->urls()) {

    // Retrieve file path
#ifndef TARGET_WINDOWS
    std::filesystem::path filePath = url.toLocalFile().toStdString();
#else
    std::filesystem::path filePath = url.toLocalFile().toStdWString();
#endif
    LOGFILE << "[I] Processing dropped file " << filePath << std::endl;

    // Verify file extension
    if (filePath.extension() != ".sppkg") {
      LOGFILE << "[E] File is not a Spplice package." << std::endl;
    }

    // Ensure a clean output directory for package extraction
    if (std::filesystem::exists(extractPath)) {
      std::filesystem::remove_all(extractPath);
    }
    std::filesystem::create_directories(extractPath);

    // Extract the file like a standard tar.xz archive
    if (!ToolsInstall::extractLocalFile(filePath, extractPath)) {
      QMessageBox::critical(nullptr, "Package Error", "Failed to extract Spplice package file, it may be corrupted.\nTry downloading it again?");
      std::filesystem::remove_all(extractPath);
      continue;
    }

    // Find and open the package manifest JSON file
    const std::filesystem::path manifestPath = extractPath / "manifest.json";
    if (!std::filesystem::exists(manifestPath)) {
      QMessageBox::critical(nullptr, "Package Error", "This Spplice package is missing a manifest file and can not be loaded.");
      std::filesystem::remove_all(extractPath);
      continue;
    }
    std::ifstream manifestFile(manifestPath);
    if (!manifestFile.is_open()) {
      QMessageBox::critical(nullptr, "Package Error", "Failed to open package manifest.");
      std::filesystem::remove_all(extractPath);
      continue;
    }

    try {

      // Read the contents of the manifest file into a string
      std::stringstream buffer;
      buffer << manifestFile.rdbuf();
      manifestFile.close();

      // Convert the string to a JSON document
      QJsonDocument doc = QJsonDocument::fromJson(QString::fromStdString(buffer.str()).toUtf8());
      QJsonObject obj = doc.object();

      // Get system time to use as archive file name
      auto now = std::chrono::system_clock::now();
      // Feed time since epoch into stringstream
      std::stringstream timess;
      timess << std::chrono::system_clock::to_time_t(now);
      // Insert this into the package object as the "file" property
      obj.insert("file", QJsonValue(QString::fromStdString(timess.str())));

      // Look for an extracted tar.xz archive and move it to its new location
      try {
        bool found = false;
        for (const auto &entry : std::filesystem::directory_iterator(extractPath)) {
          if (!entry.is_regular_file() || entry.path().extension() != ".xz") continue;
          if (entry.path().stem().extension() != ".tar") continue;
          found = true;

          std::filesystem::rename(entry.path(), archivePath / timess.str());
          break;
        }
        if (!found) {
          QMessageBox::critical(nullptr, "Package Error", "No package archive found.");
          std::filesystem::remove_all(extractPath);
          continue;
        }
      } catch (const std::filesystem::filesystem_error &e) {
        LOGFILE << "[E] Failed to process package archive: " << e.what() << std::endl;
        QMessageBox::critical(nullptr, "Package Error", "Failed to process package archive.");
        std::filesystem::remove_all(extractPath);
        return;
      }

      // Create a PackageData object from the JSON object
      ToolsPackage::PackageData *package = new ToolsPackage::PackageData(obj, "local");
      // Create a package item and insert it at the top of the package list UI
      ui->PackageListLayout->insertWidget(0, ToolsPackage::createPackageItem(package));

      // Push the object to the local package repository array
      packageArray.push_back(obj);

    } catch (const std::exception &e) {
      QMessageBox::critical(nullptr, "Package Error", "Failed to parse package manifest.");
      std::filesystem::remove_all(extractPath);
      continue;
    }

    // Clean up extracted files when done
    std::filesystem::remove_all(extractPath);

  }

  // Insert the new package array into the repository index object
  indexObject.insert("packages", packageArray);
  indexDocument.setObject(indexObject);

  // Write changes to local repository index file
  std::ofstream indexFileW(indexPath);
  if (!indexFileW) {
    LOGFILE << "[E] Failed to open local repository file " << indexPath << " for writing." << std::endl;
    QMessageBox::critical(nullptr, "Package Error", "Failed to save changes to local package repository.");
  } else {
    indexFileW << indexDocument.toJson().toStdString();
  }

  event->acceptProposedAction();

}

QVBoxLayout *MainWindow::getPackageListLayout() const {
  return ui->PackageListLayout;
}
QPushButton *MainWindow::getSettingsButton () const {
  return ui->TitleButtonL;
}
QPushButton *MainWindow::getRepositoryButton () const {
  return ui->TitleButtonR;
}

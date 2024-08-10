// Standard libraries
#include <iostream>
#include <vector>
#include <filesystem>
#include <thread>
#include <chrono>
// Main window dependencies
#include <QApplication>
#include <QFontDatabase>
#include <QThread>
#include <QObject>
#include "ui/mainwindow.h"
#include "ui/packageitem.h"
// Global CURL initialization
#include "curl/curl.h"

// Project globals
#include "globals.h"
// Project-related tools, split up into files
#include "tools/curl.h"
#include "tools/qt.h"
#include "tools/install.h"
#include "tools/package.h"
#include "tools/repo.h"

const char *repositoryURLs[] = {
  "https://p2r3.com/spplice/repo2/index.json"
};

int main (int argc, char *argv[]) {

  try {
    std::filesystem::create_directories(TEMP_DIR);
  } catch (const std::filesystem::filesystem_error& e) {
    std::cerr << "Failed to create temporary directory " << TEMP_DIR << ": " << e.what() << std::endl;
  }

  QApplication app(argc, argv);

  // Load the Quicksand font from resources
  int fontID = QFontDatabase::addApplicationFont(":/fonts/Quicksand.ttf");
  if (fontID != -1) {
    QString fontFamily = QFontDatabase::applicationFontFamilies(fontID).at(0);
    QFont font(fontFamily);
    QApplication::setFont(font);
  }

  // Set up the main application window
  QMainWindow window;
  Ui::MainWindow windowUI;
  windowUI.setupUi(&window);

  // Initialize libcurl globally
  curl_global_init(CURL_GLOBAL_DEFAULT);

  // Create a vector containing all packages from all repositories
  std::vector<ToolsPackage::PackageData> allPackages;
  // Fetch packages from each repository
  for (auto url : repositoryURLs) {
    std::vector<ToolsPackage::PackageData> repository = ToolsRepo::fetchRepository(url);
    allPackages.insert(allPackages.end(), repository.begin(), repository.end());
  }

  // Sort packages by weight
  std::sort(allPackages.begin(), allPackages.end(), [](ToolsPackage::PackageData &a, ToolsPackage::PackageData &b) {
    return a.weight > b.weight;
  });

  // Generate a PackageItem for each package
  for (auto package : allPackages) {

    // Create the package item widget
    QWidget *item = new QWidget;
    Ui::PackageItem itemUI;
    itemUI.setupUi(item);

    // Set the title and description
    itemUI.PackageTitle->setText(QString::fromStdString(package.title));
    itemUI.PackageDescription->setText(QString::fromStdString(package.description));

    // Add the item to the package list container
    windowUI.PackageListLayout->addWidget(item);

    // Start a new worker thread for asynchronous icon fetching
    PackageItemWorker *worker = new PackageItemWorker;
    QThread *workerThread = new QThread;
    worker->moveToThread(workerThread);

    std::string imageURL = package.icon;
    std::string imagePath = TEMP_DIR / (package.name + "_icon");
    QSize iconSize = itemUI.PackageIcon->size();

    // Connect the task of fetching the icon to the worker
    QObject::connect(workerThread, &QThread::started, worker, [worker, imageURL, imagePath, iconSize]() {
      QMetaObject::invokeMethod(worker, "getPackageIcon", Q_ARG(std::string, imageURL), Q_ARG(std::string, imagePath), Q_ARG(QSize, iconSize));
    });
    QObject::connect(worker, &PackageItemWorker::packageIconResult, itemUI.PackageIcon, &QLabel::setPixmap);

    // Clean up the thread once it's done
    QObject::connect(worker, &PackageItemWorker::packageIconReady, workerThread, &QThread::quit);
    QObject::connect(worker, &PackageItemWorker::packageIconReady, worker, &PackageItemWorker::deleteLater);
    QObject::connect(workerThread, &QThread::finished, workerThread, &QThread::deleteLater);

    // Start the worker thread
    workerThread->start();

    // Sleep for a few milliseconds on each package to reduce strain on the network
    std::this_thread::sleep_for(std::chrono::milliseconds(5));

  }

  // Clean up CURL
  curl_global_cleanup();

  // Display the main application window
  window.setWindowTitle("Spplice");
  window.show();

  return app.exec();

}

// Standard libraries
#include <iostream>
#include <utility>
#include <vector>
#include <filesystem>
#include <thread>
#include <chrono>
#include <cstdlib>
#include <functional>
// Main window dependencies
#include <QApplication>
#include <QFontDatabase>
#include <QThread>
#include <QObject>
#include <QDialog>
#include "ui/mainwindow.h"
#include "ui/packageitem.h"
#include "ui/packageinfo.h"

// Project globals
#include "globals.h"

// Project-related tools, split up into files
#include "tools/curl.h"
#include "tools/qt.h"
#include "tools/install.h"
#include "tools/package.h"
#include "tools/repo.h"

#include "duktape/duktape.h"

const char *repositoryURLs[] = {
  "https://p2r3.github.io/spplice-repo/index.json"
};

int main (int argc, char *argv[]) {

  try {
    std::filesystem::create_directories(TEMP_DIR);
  } catch (const std::filesystem::filesystem_error& e) {
    std::cerr << "Failed to create temporary directory " << TEMP_DIR << ": " << e.what() << std::endl;
  }

  QApplication app(argc, argv);

  // Load the Quicksand fonts from resources
  QFontDatabase::addApplicationFont(":/fonts/Quicksand-Light.ttf");
  QFontDatabase::addApplicationFont(":/fonts/Quicksand-Regular.ttf");
  QFontDatabase::addApplicationFont(":/fonts/Quicksand-Medium.ttf");

  // Set up the main application window
  QMainWindow window;
  Ui::MainWindow windowUI;
  windowUI.setupUi(&window);

  // Initialize CURL
  ToolsCURL::init();

  // Create a vector containing all packages from all repositories
  std::vector<const ToolsPackage::PackageData*> allPackages;
  // Fetch packages from each repository
  for (auto url : repositoryURLs) {
    std::vector<const ToolsPackage::PackageData*> repository = ToolsRepo::fetchRepository(url);
    allPackages.insert(allPackages.end(), repository.begin(), repository.end());
  }

  // Generate a PackageItem for each package
  for (const auto &package : allPackages) {

    // Create the package item widget
    QWidget *item = new QWidget;
    Ui::PackageItem itemUI;
    itemUI.setupUi(item);

    // Set the title and description
    itemUI.PackageTitle->setText(QString::fromStdString(package->title));
    itemUI.PackageDescription->setText(QString::fromStdString(package->description));

    // Connect the install button
    QPushButton *installButton = itemUI.PackageInstallButton;
    QObject::connect(installButton, &QPushButton::clicked, [installButton, &package]() {

      // Create a thread for asynchronous installation
      PackageItemWorker *worker = new PackageItemWorker;
      QThread *workerThread = new QThread;
      worker->moveToThread(workerThread);

      // Connect the task of installing the package to the worker
      QObject::connect(workerThread, &QThread::started, worker, [worker, &package]() {
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

    // Add the item to the package list container
    windowUI.PackageListLayout->addWidget(item);

    // Start a new worker thread for asynchronous icon fetching
    PackageItemWorker *worker = new PackageItemWorker;
    QThread *workerThread = new QThread;
    worker->moveToThread(workerThread);

    // Connect the task of fetching the icon to the worker
    QSize iconSize = itemUI.PackageIcon->size();
    QObject::connect(workerThread, &QThread::started, worker, [worker, &package, iconSize]() {
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
    QObject::connect(itemUI.PackageInfoButton, &QPushButton::clicked, [&package]() {

      QDialog *dialog = new QDialog;
      Ui::PackageInfo dialogUI;
      dialogUI.setupUi(dialog);

      // Set text data (title, author, description)
      dialogUI.PackageTitle->setText(QString::fromStdString(package->title));
      dialogUI.PackageAuthor->setText(QString::fromStdString("By " + package->author));
      dialogUI.PackageDescription->setText(QString::fromStdString(package->description));

      // Set the icon - assume the image has already been downloaded
      size_t imageURLHash = std::hash<std::string>{}(package->icon);
      std::filesystem::path imagePath = TEMP_DIR / std::to_string(imageURLHash);

      QSize iconSize = dialogUI.PackageIcon->size();

#ifndef TARGET_WINDOWS
      QPixmap iconPixmap = QPixmap(QString::fromStdString(imagePath.string())).scaled(iconSize, Qt::IgnoreAspectRatio, Qt::SmoothTransformation);
#else
      QPixmap iconPixmap = QPixmap(QString::fromStdWString(imagePath.wstring())).scaled(iconSize, Qt::IgnoreAspectRatio, Qt::SmoothTransformation);
#endif

      QPixmap iconRoundedPixmap = ToolsQT::getRoundedPixmap(iconPixmap, 10);
      dialogUI.PackageIcon->setPixmap(iconRoundedPixmap);

      dialog->setWindowTitle(QString::fromStdString("Details for " + package->title));
      dialog->exec();

    });

    // Sleep for a few milliseconds on each package to reduce strain on the network
    std::this_thread::sleep_for(std::chrono::milliseconds(5));

  }

  // Clean up CURL on program termination
  std::atexit(ToolsCURL::cleanup);

  // Display the main application window
  window.setWindowTitle("Spplice");
  window.show();

  // Ensure that no package is installed if Portal 2 is running when exiting Spplice
#ifndef TARGET_WINDOWS
  std::atexit([]() {
    const std::string gameProcessPath = ToolsInstall::getProcessPath("portal2_linux");
    if (gameProcessPath.length() == 0) return;
    ToolsInstall::Uninstall((std::filesystem::path(gameProcessPath).parent_path()).string());
  });
#else
  std::atexit([]() {
    const std::wstring gameProcessPath = ToolsInstall::getProcessPath("portal2.exe");
    if (gameProcessPath.length() == 0) return;
    ToolsInstall::Uninstall((std::filesystem::path(gameProcessPath).parent_path()).wstring());
  });
#endif

  return app.exec();

}

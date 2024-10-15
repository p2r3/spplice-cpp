// Standard libraries
#include <iostream>
#include <utility>
#include <vector>
#include <deque>
#include <filesystem>
#include <thread>
#include <chrono>
#include <cstdlib>
#include <functional>
// Main window dependencies
#include <QApplication>
#include <QCoreApplication>
#include <QFontDatabase>
#include <QObject>
#include <QDialog>
#include "ui/mainwindow.h"
#include "ui/addrepository.h"

// Project globals
#include "globals.h"

// Project-related tools, split up into files
#include "tools/curl.h"
#include "tools/qt.h"
#include "tools/install.h"
#include "tools/package.h"
#include "tools/repo.h"

std::deque<std::string> repositoryURLs = {
  "https://p2r3.github.io/spplice-repo/index.json"
};

void addRepository (const std::string url, QVBoxLayout *container) {

  // Fetch the repository packages
  std::vector<const ToolsPackage::PackageData*> repository = ToolsRepo::fetchRepository(url);

  // Keep track of added package count to order them properly
  int currPackage = 0;

  for (const ToolsPackage::PackageData *package : repository) {
    // Create PackageItem widget from PackageData
    QWidget *item = ToolsPackage::createPackageItem(package);
    // Add the item to the package list container
    container->insertWidget(currPackage, item);
    currPackage ++;
    // Sleep for a few milliseconds on each package to reduce strain on the network
    std::this_thread::sleep_for(std::chrono::milliseconds(5));
  }

}

int main (int argc, char *argv[]) {

  try {
    std::filesystem::create_directories(TEMP_DIR);
  } catch (const std::filesystem::filesystem_error& e) {
    std::cerr << "Failed to create temporary directory " << TEMP_DIR << ": " << e.what() << std::endl;
  }

  qputenv("QT_FONT_DPI", QByteArray("96"));

  QCoreApplication::setAttribute(Qt::AA_EnableHighDpiScaling);
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

  QVBoxLayout *packageContainer = windowUI.PackageListLayout;

  // Connect the "Add Repository" button
  QObject::connect(windowUI.TitleButtonR, &QPushButton::clicked, [packageContainer]() {

    // Create new repository entry dialog
    QDialog *dialog = new QDialog;
    Ui::RepoDialog dialogUI;
    dialogUI.setupUi(dialog);

    QLineEdit *urlTextBox = dialogUI.RepoURL;

    // Connect the "OK" button
    QObject::connect(dialogUI.DialogButton, &QPushButton::clicked, [packageContainer, urlTextBox, dialog]() {
      addRepository(urlTextBox->text().toStdString(), packageContainer);
      dialog->hide();
    });

    // Connect the event of pressing return
    QObject::connect(urlTextBox, &QLineEdit::returnPressed, [packageContainer, urlTextBox, dialog]() {
      addRepository(urlTextBox->text().toStdString(), packageContainer);
      dialog->hide();
    });

    dialog->exec();

  });

  // Fetch packages from each repository
  for (const std::string &url : repositoryURLs) {
    addRepository(url, packageContainer);
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

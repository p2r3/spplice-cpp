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
#include <QCoreApplication>
#include <QFontDatabase>
#include <QObject>
#include <QDialog>
#include "ui/mainwindow.h"
#include "ui/repositories.h"

// Project globals
#include "globals.h"

// Project-related tools, split up into files
#include "tools/curl.h"
#include "tools/qt.h"
#include "tools/install.h"
#include "tools/package.h"
#include "tools/repo.h"

// Fetch and display packages from the given repository URL
void displayRepository (const std::string &url, QVBoxLayout *container) {

  // Fetch the repository packages // TODO: Make this asynchronous
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

// Remove all packages of the given repository URL from the list
void hideRepository (const std::string &url, QVBoxLayout *container) {

  for (int i = 0; i < container->count(); i ++) {
    QWidget *child = container->itemAt(i)->widget();
    if (child->property("packageRepository").toString().toStdString() == url) {
      container->removeWidget(child);
      delete child;
      i --;
    }
  }

}

int main (int argc, char *argv[]) {

  try { // Ensure CACHE_DIR exists
    std::filesystem::create_directories(CACHE_DIR);
  } catch (const std::filesystem::filesystem_error& e) {
    std::cerr << "Failed to create temporary directory " << CACHE_DIR << ": " << e.what() << std::endl;
  }

  try { // Ensure APP_DIR exists
    std::filesystem::create_directories(APP_DIR);
  } catch (const std::filesystem::filesystem_error& e) {
    std::cerr << "Failed to create application directory " << APP_DIR << ": " << e.what() << std::endl;
  }

  // Set up high-DPI scaling
  QApplication::setAttribute(Qt::AA_EnableHighDpiScaling);
  QApplication::setHighDpiScaleFactorRoundingPolicy(Qt::HighDpiScaleFactorRoundingPolicy::PassThrough);

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

    // Create repository management dialog
    QDialog *dialog = new QDialog;
    Ui::RepoDialog dialogUI;
    dialogUI.setupUi(dialog);

    QLineEdit *urlInput = dialogUI.AddInput;
    QComboBox *dropdown = dialogUI.RemoveDropdown;

    // List external repositories in dropdown
    std::vector<std::string> repositories = ToolsRepo::readFromFile();
    for (const std::string &url : repositories) {
      dropdown->addItem(QString::fromStdString(url));
    }

    // Define URL submit behavior
    auto submitURL = [packageContainer, urlInput, dialog]() {
      const std::string url = urlInput->text().toStdString();
      displayRepository(url, packageContainer);
      ToolsRepo::writeToFile(url);
      dialog->hide();
    };

    // Connect the "Add" button and text input return event
    QObject::connect(dialogUI.AddButton, &QPushButton::clicked, submitURL);
    QObject::connect(urlInput, &QLineEdit::returnPressed, submitURL);

    // Connect the "Remove" button
    QObject::connect(dialogUI.RemoveButton, &QPushButton::clicked, [packageContainer, dropdown, dialog]() {
      const std::string url = dropdown->currentText().toStdString();
      hideRepository(url, packageContainer);
      ToolsRepo::removeFromFile(url);
      dialog->hide();
    });

    dialog->exec();

  });

  // Display the main application window
  window.setWindowTitle("Spplice");
  window.show();

  // Load the global repository
  displayRepository("https://p2r3.github.io/spplice-repo/index.json", packageContainer);

  // Load additional repositories from file
  std::vector<std::string> repositories = ToolsRepo::readFromFile();
  for (const std::string &url : repositories) {
    displayRepository(url, packageContainer);
  }

  // Clean up CURL on program termination
  std::atexit(ToolsCURL::cleanup);

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

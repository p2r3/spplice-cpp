// Standard libraries
#include <iostream>
#include <fstream>
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
#include <QMessageBox>
#include <QtConcurrent>
#include <QFutureWatcher>
#include "ui/mainwindow.h"
#include "ui/repositories.h"
#include "ui/settings.h"

// Project globals
#include "globals.h"

// Project-related tools, split up into files
#include "tools/curl.h"
#include "tools/qt.h"
#include "tools/install.h"
#include "tools/package.h"
#include "tools/repo.h"

// Fetch and display packages from the given repository URL asynchronously
void displayRepository (const std::string &url, const std::string &last, QVBoxLayout *container) {

  // Set up a watcher to fetch repository packages asynchronously
  QFutureWatcher<std::vector<const ToolsPackage::PackageData*>> *watcher;
  watcher = new QFutureWatcher<std::vector<const ToolsPackage::PackageData*>>(container);

  // Connect a lambda that adds the items when they've been fetched
  QObject::connect(watcher, &QFutureWatcher<std::vector<const ToolsPackage::PackageData*>>::finished, container, [container, watcher, last]() {
    std::vector<const ToolsPackage::PackageData*> repository = watcher->result();

    // Convert last fetched repository url to QString for easier comparisons
    QString lastQstr = QString::fromStdString(last);
    // Keep track insertion point to order packages properly
    int insertAt = 0;

    // Move insertion point to the top of the most recently fetched repository
    while (insertAt < container->count()) {
      QWidget *widget = container->itemAt(insertAt)->widget();
      if (widget->property("packageRepository").toString() == lastQstr) break;
      insertAt ++;
    }

    for (const ToolsPackage::PackageData *package : repository) {
      // Create PackageItem widget from PackageData
      QWidget *item = ToolsPackage::createPackageItem(package);
      // Add the item to the package list container
      container->insertWidget(insertAt, item);
      insertAt ++;
      // Sleep for a few milliseconds on each package to reduce strain on the network
      std::this_thread::sleep_for(std::chrono::milliseconds(5));
    }

    watcher->deleteLater();
  });

  // Fetch the repository packages in a new thread
  QFuture<std::vector<const ToolsPackage::PackageData*>> future = QtConcurrent::run([url]() {
    return ToolsRepo::fetchRepository(url);
  });
  watcher->setFuture(future);

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

// Check if a custom cache directory has been specified
void checkCacheOverride (const std::filesystem::path &configPath) {

  if (!std::filesystem::exists(configPath)) return;

  std::ifstream configFile(configPath);
  if (!configFile.is_open()) {
    std::cerr << "[E] Failed to open " << configPath.c_str() << " for reading." << std::endl;
    return;
  }

  std::string customCacheDir;
  std::getline(configFile, customCacheDir);
  configFile.close();

  if (!std::filesystem::exists(customCacheDir)) {
    std::cerr << "[E] Invalid cache directory " << customCacheDir.c_str() << "." << std::endl;
    return;
  }

  CACHE_DIR = std::filesystem::path(customCacheDir);

}

// A link to the index for the global Spplice repository
const std::string globalRepository = "https://p2r3.github.io/spplice-repo/index.json";

int main (int argc, char *argv[]) {

  // Check for a CACHE_DIR override in cache_dir.txt
  checkCacheOverride(APP_DIR / "cache_dir.txt");

  // Check if caching has been disabled
  CACHE_ENABLE = !std::filesystem::exists(APP_DIR / "disable_cache");

  try { // Ensure CACHE_DIR exists
    std::filesystem::create_directories(CACHE_DIR);
  } catch (const std::filesystem::filesystem_error& e) {
    std::cerr << "[E] Failed to create temporary directory " << CACHE_DIR.c_str() << ": " << e.what() << std::endl;
  }

  try { // Ensure APP_DIR exists
    std::filesystem::create_directories(APP_DIR);
  } catch (const std::filesystem::filesystem_error& e) {
    std::cerr << "[E] Failed to create application directory " << APP_DIR.c_str() << ": " << e.what() << std::endl;
  }
  // Open the log file
  LOGFILE = std::ofstream(APP_DIR / "log.txt");

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

  // Connect the "Settings" button
  QObject::connect(windowUI.TitleButtonL, &QPushButton::clicked, [packageContainer]() {

    // Create settings dialog
    QDialog *dialog = new QDialog;
    Ui::SettingsDialog dialogUI;
    dialogUI.setupUi(dialog);

    // Set the text of the cache toggle button
    dialogUI.CacheToggleBtn->setText(CACHE_ENABLE ? "Disable cache" : "Enable cache");

    // Connect the "Clear cache" button
    QObject::connect(dialogUI.CacheClearBtn, &QPushButton::clicked, []() {
      std::filesystem::remove_all(CACHE_DIR);
      std::filesystem::create_directories(CACHE_DIR);
      QMessageBox::information(nullptr, "Cache Cleared", "Cache has been cleared successfully.");
    });

    QPushButton *cacheToggle = dialogUI.CacheToggleBtn;

    // Connect the cache toggle button
    QObject::connect(cacheToggle, &QPushButton::clicked, [cacheToggle]() {

      // Toggle the caching behavior
      CACHE_ENABLE = !CACHE_ENABLE;

      // Save the cache state to file
      if (!CACHE_ENABLE) {
        std::ofstream disableCacheFile(APP_DIR / "disable_cache");
        if (!disableCacheFile.is_open()) {
          LOGFILE << "[E] Failed to create disable_cache file." << std::endl;
        }
      } else {
        std::filesystem::remove(APP_DIR / "disable_cache");
      }

      cacheToggle->setText(CACHE_ENABLE ? "Disable cache" : "Enable cache");
      QMessageBox::information(nullptr, "Cache Toggled", CACHE_ENABLE ? "Cache has been enabled." : "Cache has been disabled.");

    });

    QLineEdit *cacheInput = dialogUI.CacheDirInput;

    // Write the cache directory to the input field
#ifndef TARGET_WINDOWS
    cacheInput->setText(QString::fromStdString(CACHE_DIR.string()));
#else
    cacheInput->setText(QString::fromStdWString(CACHE_DIR.wstring()));
#endif

    // Connect the cache directory "Apply" button
    QObject::connect(dialogUI.CacheDirBtn, &QPushButton::clicked, [cacheInput]() {

#ifndef TARGET_WINDOWS
      const std::filesystem::path newPath(cacheInput->text().toStdString());
#else
      const std::filesystem::path newPath(cacheInput->text().toStdWString());
#endif

      if (newPath == CACHE_DIR) return;
      CACHE_DIR = newPath;

      try { // Ensure the new directory is a valid path
        std::filesystem::create_directories(newPath);
      } catch (const std::filesystem::filesystem_error& e) {
        QMessageBox::critical(nullptr, "Cache Directory Error", "Failed to create cache directory: " + QString::fromStdString(e.what()));
        return;
      }

      // Write the new cache directory to the config file
      std::ofstream configFile(APP_DIR / "cache_dir.txt");
      if (!configFile.is_open()) {
        LOGFILE << "[E] Failed to open cache config file for writing." << std::endl;
      } else {
        configFile << CACHE_DIR.string();
      }

      QMessageBox::information(nullptr, "Cache Directory Updated", "The cache directory has been updated successfully.");

    });

    dialog->open();

  });

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

      // Insert the new repository before the topmost repository
      QWidget *widget = packageContainer->itemAt(0)->widget();
      displayRepository(url, widget->property("packageRepository").toString().toStdString(), packageContainer);

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

    dialog->open();

  });

  // Display the main application window
  window.setWindowTitle("Spplice");
  window.show();

  // Load the global repository, putting it at the very bottom
  displayRepository(globalRepository, "", packageContainer);

  // Keep track of the previous repository to order them when fetching asynhronously
  std::string lastRepository = globalRepository;

  // Load additional repositories from file
  std::vector<std::string> repositories = ToolsRepo::readFromFile();
  for (const std::string &url : repositories) {
    displayRepository(url, lastRepository, packageContainer);
    lastRepository = url;
  }

  // Clean up CURL on program termination
  std::atexit(ToolsCURL::cleanup);

  // Ensure that no package is installed if Portal 2 is running when exiting Spplice
  std::atexit(ToolsInstall::uninstall);

  return app.exec();

}

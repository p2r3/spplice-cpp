// Standard libraries
#include <iostream>
#include <vector>
#include <filesystem>
// Main window dependencies
#include <QApplication>
#include "ui/mainwindow.h"
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

  // Generate a PackageItem for each package and add it to the layout
  for (auto package : allPackages) {
    windowUI.PackageListLayout->addWidget(package.createPackageItem());
  }

  // Clean up CURL
  curl_global_cleanup();

  // Display the main application window
  window.setWindowTitle("Spplice");
  window.show();

  return app.exec();

}

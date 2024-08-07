// Standard libraries
#include <iostream>
#include <string>
#include <vector>
#include <fstream>
#include <filesystem>
// Qt window drawing dependencies
#include <QApplication>
#include <QPainter>
#include <QPainterPath>
// Widget templates for window elements
#include "ui/mainwindow.h"
#include "ui/packageitem.h"
// JSON parsing
#include "rapidjson/document.h"
#include "rapidjson/writer.h"
#include "rapidjson/stringbuffer.h"
// Network requests
#include "curl/curl.h"

// Points to the system-specific designated temporary file path
const std::filesystem::path TEMP_DIR = std::filesystem::temp_directory_path() / "cpplice";

// Project-related tools, split up into files
#include "tools/curl.cpp"
#include "tools/qt.cpp"
#include "tools/package.cpp"
#include "tools/repo.cpp"

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

  // Initialize libcurl for package list fetching
  CURL *curl = curl_easy_init();

  if (!curl) {
    throw std::runtime_error("Failed to initialize CURL");
  }

  // Create a vector containing all packages from all repositories
  std::vector<PackageData> allPackages;
  // Fetch packages from each repository
  for (auto url : repositoryURLs) {
    std::vector<PackageData> repository = fetchRepository(curl, url);
    allPackages.insert(allPackages.end(), repository.begin(), repository.end());
  }

  // Sort packages by weight
  std::sort(allPackages.begin(), allPackages.end(), [](PackageData &a, PackageData &b) {
    return a.weight > b.weight;
  });

  // Generate a PackageItem for each package and add it to the layout
  for (auto package : allPackages) {
    windowUI.PackageListLayout->addWidget(package.createPackageItem(curl));
  }

  // Clean up CURL
  curl_easy_cleanup(curl);

  // Display the main application window
  window.setWindowTitle("Spplice");
  window.show();

  return app.exec();

}

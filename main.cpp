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

// CURL write callback function for appending to a string
size_t curlStringWriteCallback (void *contents, size_t size, size_t nmemb, void *userp) {
  ((std::string*)userp)->append((char*)contents, size *nmemb);
  return size *nmemb;
}

// CURL write callback function for writing to a file
size_t curlFileWriteCallback(void *contents, size_t size, size_t nmemb, void *userp) {
  std::ofstream *ofs = static_cast<std::ofstream *>(userp);
  size_t totalSize = size * nmemb;
  ofs->write(static_cast<const char *>(contents), totalSize);
  return totalSize;
}

// Points to the system-specific designated temporary file path
const std::filesystem::path TEMP_DIR = std::filesystem::temp_directory_path()/"cpplice";

// Downloads a file to the application's temp directory from the specified URL, returns path to file
std::string downloadTempFile (CURL *curl, const std::string &url, const std::string &filename) {

  std::filesystem::path outputPath = TEMP_DIR/filename;

  std::ofstream ofs(outputPath, std::ios::binary);

  if (!ofs.is_open()) {
    std::cerr << "Failed to open file for writing: " << outputPath << std::endl;
    return nullptr;
  }

  curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
  curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, curlFileWriteCallback);
  curl_easy_setopt(curl, CURLOPT_WRITEDATA, &ofs);

  CURLcode response = curl_easy_perform(curl);

  if (response != CURLE_OK) {
    std::cerr << "Failed to download file from \"" << url << "\": " << curl_easy_strerror(response) << std::endl;
    return nullptr;
  }

  return outputPath;

}

// Returns a version of the input pixmap with rounded corners
QPixmap getRoundedPixmap (const QPixmap &src, int radius) {

  // Create a new QPixmap with the same size as the source pixmap
  QPixmap rounded(src.size());
  rounded.fill(Qt::transparent); // Fill with transparency

  // Set up QPainter to draw on the rounded pixmap
  QPainter painter(&rounded);
  painter.setRenderHint(QPainter::Antialiasing);
  painter.setRenderHint(QPainter::SmoothPixmapTransform);

  // Create a path for the rounded corners
  QPainterPath path;
  path.addRoundedRect(src.rect(), radius, radius);

  // Clip the painter to the rounded rectangle
  painter.setClipPath(path);

  // Draw the original pixmap onto the rounded pixmap
  painter.drawPixmap(0, 0, src);

  return rounded;

}

// Define the structure of a package's properties
class PackageData {
  private:
    std::string name;
    std::string title;
    std::string author;
    std::string file;
    std::string icon;
    std::string description;
    int weight;
  public:
    // Constructs the PackageData instance from a JSON object
    PackageData (rapidjson::Value &package) {
      this->name = package["name"].GetString();
      this->title = package["title"].GetString();
      this->author = package["author"].GetString();
      this->file = package["file"].GetString();
      this->icon = package["icon"].GetString();
      this->description = package["description"].GetString();
      this->weight = package["weight"].GetInt();
    }

    int getWeight() { return this->weight; }

    // Generates a PackageItem instance of this package for rendering
    QWidget *createPackageItem(CURL *curl) {

      QWidget *item = new QWidget;
      Ui::PackageItem itemUI;
      itemUI.setupUi(item);

      // Set values of text-based elements
      itemUI.PackageTitle->setText(QString::fromStdString( this->title ));
      itemUI.PackageDescription->setText(QString::fromStdString( this->description ));

      // Download the package icon
      std::string imageFileName = this->name + "_icon";
      QString imagePath = QString::fromStdString(downloadTempFile(curl, this->icon, imageFileName));

      // Create a pixmap for the icon
      QSize labelSize = itemUI.PackageIcon->size();
      QPixmap iconPixmap = QPixmap(imagePath).scaled(labelSize, Qt::IgnoreAspectRatio, Qt::SmoothTransformation);
      QPixmap iconRoundedPixmap = getRoundedPixmap(iconPixmap, 10);

      // Set the package icon
      itemUI.PackageIcon->setPixmap(iconRoundedPixmap);

      return item;

    }
};

// Fetch and parse repository JSON from the given URL
std::vector<PackageData> fetchRepository (CURL *curl, const char *url) {

  std::string readBuffer;

  // Set the URL for the request
  curl_easy_setopt(curl, CURLOPT_URL, url);

  // Set the callback function to handle the data
  curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, curlStringWriteCallback);
  curl_easy_setopt(curl, CURLOPT_WRITEDATA, &readBuffer);

  // Perform the request
  CURLcode response = curl_easy_perform(curl);

  // Check for errors
  if (response != CURLE_OK) {
    std::cerr << "Failed to fetch repository \"" << url << "\": " << curl_easy_strerror(response) << std::endl;
    // Soft-fail by returning an empty vector
    return std::vector<PackageData>();
  }

  // Parse the repository index
  rapidjson::Document doc;
  doc.Parse(readBuffer.c_str());

  // Count the packages in the repository
  rapidjson::Value &packages = doc["packages"];
  const int packageCount = packages.Size();

  // Create a vector, since a dynamic array is a bit more of a pain in the ass
  std::vector<PackageData> repository;
  for (int i = 0; i < packageCount; i ++) {
    repository.push_back(PackageData(packages[i]));
  }

  return repository;

}

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
    return a.getWeight() > b.getWeight();
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

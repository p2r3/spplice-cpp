// Standard libraries
#include <iostream>
#include <string>
#include <vector>
#include <fstream>
#include <filesystem>
// Qt window drawing dependencies
#include <QApplication>
#include <QWidget>
#include <QPushButton>
#include <QMessageBox>
#include <QLabel>
#include <QVBoxLayout>
#include <QScrollArea>
// Widget templates for window elements
#include "ui/mainwindow.h"
#include "ui/packageitem.h"
// Used for loading the QSS file
#include <QFile>
#include <QTextStream>
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

// Defines a single package container in the package list
class _PackageItem : public QWidget {
  public:
    _PackageItem (CURL *curl, const std::string& name, const QString &title, const QString &description, const std::string &imageUrl, QWidget *parent) : QWidget(parent) {
      QHBoxLayout *layout = new QHBoxLayout(this);

      std::string imageFileName = name + "_icon";
      QString imagePath = QString::fromStdString(downloadTempFile(curl, imageUrl, imageFileName));

      QLabel *imageLabel = new QLabel();
      imageLabel->setPixmap(QPixmap(imagePath).scaled(100, 100, Qt::KeepAspectRatio));
      layout->addWidget(imageLabel);

      QVBoxLayout *textLayout = new QVBoxLayout();
      QLabel *titleLabel = new QLabel(title);
      QLabel *descriptionLabel = new QLabel(description);
      textLayout->addWidget(titleLabel);
      textLayout->addWidget(descriptionLabel);
      layout->addLayout(textLayout);

      QPushButton *button = new QPushButton(">");
      layout->addWidget(button);

      setLayout(layout);
    }
};

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

    // Generates a PackageItem instance of this package for rendering
    void createPackageItem(CURL *curl, QWidget *parent = nullptr) {

      QWidget item(parent);
      Ui::PackageItem itemUI;
      itemUI.setupUi(&item);

      itemUI.PackageTitle->setText(QString::fromStdString( this->title ));
      itemUI.PackageDescription->setText(QString::fromStdString( this->description ));

      std::string imageFileName = this->name + "_icon";
      QString imagePath = QString::fromStdString(downloadTempFile(curl, this->icon, imageFileName));

      // itemUI.PackageIcon->setPixmap(QPixmap(imagePath).scaled(100, 100, Qt::KeepAspectRatio));
      itemUI.PackageIcon->setPixmap(QPixmap(imagePath));

    }
};

// Fetch and parse repository JSON from the given URL
std::vector<PackageData> fetchRepository (CURL *curl, const std::string &url) {

  std::string readBuffer;

  // Set the URL for the request
  curl_easy_setopt(curl, CURLOPT_URL, url.c_str());

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
  std::vector<std::vector<PackageData>> repositories;
  // Fetch the global package repository
  repositories.push_back(fetchRepository(curl, "https://p2r3.com/spplice/repo2/index.json"));

  // Generate a _PackageItem for each package and add it to the layout
  for (auto repository : repositories) {
    for (auto package : repository) {
      package.createPackageItem(curl, windowUI.PackageListContents);
    }
  }

  // Clean up CURL
  curl_easy_cleanup(curl);

  // Display the main application window
  window.setWindowTitle("Spplice");
  window.show();

  return app.exec();

}

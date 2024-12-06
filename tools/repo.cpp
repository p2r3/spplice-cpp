#include <iostream>
#include <fstream>
#include <filesystem>
#include <vector>
#include <string>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>

#include "../globals.h"
#include "curl.h" // ToolsCURL
#include "package.h" // ToolsPackage

// Definitions for this source file
#include "repo.h"

// Fetches and parses repository JSON from the given URL
std::vector<const ToolsPackage::PackageData*> ToolsRepo::fetchRepository (const std::string &url) {

  // Download the repository JSON
  QString jsonString = QString::fromStdString(ToolsCURL::downloadString(url));

  // Parse the repository index
  QJsonDocument doc = QJsonDocument::fromJson(jsonString.toUtf8());
  QJsonObject obj = doc.object();

  // Count the packages in the repository
  QJsonArray packages = obj["packages"].toArray();
  const int packageCount = packages.size();

  // Create a vector, since a dynamic array is a bit more of a pain in the ass
  std::vector<const ToolsPackage::PackageData*> repository;
  for (int i = 0; i < packageCount; i ++) {
    ToolsPackage::PackageData *package = new ToolsPackage::PackageData(packages[i].toObject(), url);
    repository.push_back(package);
  }

  return repository;

}

// Adds the given URL to the repository list file
void ToolsRepo::writeToFile (const std::string &url) {

  // Read the repository list
  std::ifstream readFile(REPO_PATH);

  // Exit early if the URL is already in the repository list
  if (readFile.is_open()) {
    std::string line;
    while (std::getline(readFile, line)) {
      if (line == url) return;
    }
  } else {
    LOGFILE << "[E] Failed to open " << REPO_PATH << " for reading." << std::endl;
  }

  std::ofstream file(REPO_PATH, std::ios::app);

  // Append the URL to the end of the file
  if (file.is_open()) {
    file << url << std::endl;
    file.close();
  } else {
    LOGFILE << "[E] Failed to open " << REPO_PATH << " for writing." << std::endl;
  }

}

// Returns a list of repositories stored in the file
std::vector<std::string> ToolsRepo::readFromFile () {

  std::vector<std::string> output;

  // Check if the file exists
  if (!std::filesystem::exists(REPO_PATH)) {
    LOGFILE << REPO_PATH << " does not exist, creating it..." << std::endl;

    // If it doesn't, write a blank file and exit
    std::ofstream file(REPO_PATH);
    if (!file.is_open()) {
      LOGFILE << "[E] Failed to create " << REPO_PATH << std::endl;
    }

    return output;
  }

  // Read the repository list
  std::ifstream file(REPO_PATH);

  if (!file.is_open()) {
    LOGFILE << "[E] Failed to open " << REPO_PATH << " for reading." << std::endl;
    return output;
  }

  // Each line is a repository URL, add it to the container
  std::string line;
  while (std::getline(file, line)) {
    output.push_back(line);
  }

  file.close();
  return output;

}

// Removes the given URL from the repository list file
void ToolsRepo::removeFromFile (const std::string &url) {

  // Check if the file exists
  if (!std::filesystem::exists(REPO_PATH)) {
    LOGFILE << REPO_PATH << " does not exist." << std::endl;
    return;
  }

  // Use a temporary file to store the new repository list
  const std::filesystem::path tempPath = REPO_PATH.string() + ".tmp";
  std::ofstream tempFile(tempPath);

  if (!tempFile.is_open()) {
    LOGFILE << "[E] Failed to open " << tempPath << " for writing." << std::endl;
    return;
  }

  // Read the repository list
  std::ifstream file(REPO_PATH);

  if (!file.is_open()) {
    LOGFILE << "[E] Failed to open " << REPO_PATH << " for reading." << std::endl;
    return;
  }

  // Each line is a repository URL, add it to the container
  std::string line;
  while (std::getline(file, line)) {
    if (line == url) continue;
    tempFile << line << std::endl;
  }

  file.close();
  tempFile.close();

  // Replace the original file with the temporary one
  std::filesystem::remove(REPO_PATH);
  std::filesystem::rename(REPO_PATH.string() + ".tmp", REPO_PATH);

}

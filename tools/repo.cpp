#include <iostream>
#include <vector>
#include <string>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
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
    repository.push_back(new ToolsPackage::PackageData(packages[i].toObject()));
  }

  return repository;

}

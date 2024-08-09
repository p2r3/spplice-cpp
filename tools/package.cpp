#include <string>
#include "rapidjson/document.h"

// Definitions for this source file
#include "package.h"

// Constructs the PackageData instance from a JSON object
ToolsPackage::PackageData::PackageData (rapidjson::Value &package) {
  this->name = package["name"].GetString();
  this->title = package["title"].GetString();
  this->author = package["author"].GetString();
  this->file = package["file"].GetString();
  this->icon = package["icon"].GetString();
  this->description = package["description"].GetString();
  this->weight = package["weight"].GetInt();
}

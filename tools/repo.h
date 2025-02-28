#ifndef TOOLS_REPO_H
#define TOOLS_REPO_H

#include <vector>
#include "package.h" // ToolsPackage

class ToolsRepo {
  public:
    static std::vector<const ToolsPackage::PackageData*> parseRepository (const std::string &json, const std::string &url = "local");
    static std::vector<const ToolsPackage::PackageData*> fetchRepository (const std::string &url);
    static void writeToFile (const std::string &url);
    static std::vector<std::string> readFromFile ();
    static void removeFromFile (const std::string &url);
};

#endif

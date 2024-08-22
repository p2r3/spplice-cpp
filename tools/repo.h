#ifndef TOOLS_REPO_H
#define TOOLS_REPO_H

#include <vector>
#include "package.h" // ToolsPackage

class ToolsRepo {
  public:
    static std::vector<const ToolsPackage::PackageData*> fetchRepository (const std::string &url);
};

#endif

#ifndef TOOLS_REPO_H
#define TOOLS_REPO_H

class ToolsRepo {
  public:
    static std::vector<ToolsPackage::PackageData> fetchRepository (const std::string &url);
};

#endif

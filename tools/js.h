#ifndef TOOLS_JS_H
#define TOOLS_JS_H

#include <filesystem>

class ToolsJS {
  public:
    static void runFile (const std::filesystem::path &filePath);
};

#endif

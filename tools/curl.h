#ifndef TOOLS_CURL_H
#define TOOLS_CURL_H

#include <filesystem>

class ToolsCURL {
  public:
    static void init ();
    static void cleanup ();

    static size_t curlStringWriteCallback (void *contents, size_t size, size_t nmemb, void *userp);
    static size_t curlFileWriteCallback(void *contents, size_t size, size_t nmemb, void *userp);

    static bool downloadFile (const std::string &url, const std::filesystem::path outputPath);
    static std::string downloadString (const std::string &url);
};

#endif

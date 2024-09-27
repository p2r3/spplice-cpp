#ifndef TOOLS_CURL_H
#define TOOLS_CURL_H

#include <filesystem>

#ifndef TARGET_WINDOWS
  // If on Linux, include the system's CURL
  #include "curl/curl.h"
#else
  // If on Windows, include the CURL from windeps.sh
  #include "../win/include/curl/curl.h"
#endif

class ToolsCURL {
  public:
    static void init ();
    static void cleanup ();

    static bool downloadFile (const std::string &url, const std::filesystem::path outputPath);
    static std::string downloadString (const std::string &url);

    static CURL* wsConnect (const std::string &url);
    static void wsDisconnect (CURL *curl);
    static bool wsSend (CURL *curl, const std::string &message);
    static std::string wsReceive (CURL *curl);
};

#endif

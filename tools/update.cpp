#include <iostream>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QMessageBox>

#ifdef TARGET_WINDOWS
  #include <windows.h>
#else
  #include <unistd.h>
  #include <limits.h>
#endif

#include "curl.h" // ToolsCURL
#include "../globals.h" // Project globals

// Definitions for this source file
#include "update.h"

#ifndef TARGET_WINDOWS
  const std::string updateBinary = "SppliceCPP";
#else
  const std::string updateBinary = "_autoupdate";
#endif

std::filesystem::path getExecutablePath () {

#ifdef TARGET_WINDOWS
  wchar_t buffer[MAX_PATH];
  DWORD size = GetModuleFileNameW(NULL, buffer, MAX_PATH);
  if (size == 0 || size == MAX_PATH) return std::filesystem::path();
#else
  char buffer[PATH_MAX];
  ssize_t size = readlink("/proc/self/exe", buffer, sizeof(buffer) - 1);
  if (size == -1) return std::filesystem::path();
  buffer[size] = '\0';
#endif

  return std::filesystem::path(buffer);

}

void ToolsUpdate::installUpdate () {

  // Download the GitHub releases manifest containing one item
  std::string releasesString = ToolsCURL::downloadString("https://api.github.com/repos/p2r3/spplice-cpp/releases?per_page=1");
  // If the download failed, do nothing
  if (releasesString == "") {
    LOGFILE << "[E] Failed to download GitHub releases list" << std::endl;
    return;
  }

  // Parse the JSON to get an array of releases
  QJsonDocument doc = QJsonDocument::fromJson(QString::fromStdString(releasesString).toUtf8());
  QJsonArray releases = doc.array();
  // Get the latest (pre)release
  QJsonObject latest = releases[0].toObject();

  // If we're up to date, do nothing
  const std::string latestTag = latest["tag_name"].toString().toStdString();
  if (SPPLICE_VERSION_TAG == latestTag) return;
  LOGFILE << "[I] Update required (" << SPPLICE_VERSION_TAG << " -> " << latestTag << ")" << std::endl;

  // Parse the assets array
  QJsonArray assets = latest["assets"].toArray();
  // Find the right binary for this platform
  QJsonObject asset;
  bool foundUpdateBinary = false;
  for (const QJsonValue &curr : assets) {
    asset = curr.toObject();
    if (asset["name"].toString().toStdString() == updateBinary) {
      foundUpdateBinary = true;
      break;
    }
  }
  // If no matching update binary was found, request a manual update
  if (!foundUpdateBinary) {
    QMessageBox::information(nullptr, "Spplice Update",
      "This version of Spplice is obsolete and requires a manual update.\nGo to p2r3.com/spplice to download the latest version.");
    return;
  }

  // Download the binary
  const std::string url = asset["browser_download_url"].toString().toStdString();
  LOGFILE << "[I] Downloading update from \"" << url << '"' << std::endl;

  const std::filesystem::path updatePath = CACHE_DIR / "update_binary";
  bool success = ToolsCURL::downloadFile(url, updatePath);
  // If the download failed, do nothing
  if (!success) {
    LOGFILE << "[E] Failed to download update from \"" << url << '"' << std::endl;
    return;
  }

  // Get the path of the currently running executable
  const std::filesystem::path executablePath = getExecutablePath();
  // If this failed, do nothing
  if (executablePath.empty()) {
    LOGFILE << "[E] Failed to find own executable." << std::endl;
    return;
  }
  LOGFILE << "[I] Found own executable at " << executablePath << std::endl;

  // Swap the running executable with the one we just downloaded
#ifdef TARGET_WINDOWS
  // If on Windows, defer replacing until next reboot to avoid conflicts
  if (MoveFileExW(updatePath.c_str(), executablePath.c_str(), MOVEFILE_DELAY_UNTIL_REBOOT)) {
    LOGFILE << "[I] Scheduled update file replacement for next reboot" << std::endl;
  } else {
    LOGFILE << "[E] Failed to schedule update file replacement. ERRNO: " << GetLastError() << std::endl;
    return;
  }
  QMessageBox::information(nullptr, "Spplice Update",
    "Spplice has been updated.\nChanges will take effect after a system reboot.");
#else
  // If on Linux, replace the executable right away
  try {
    std::filesystem::remove(executablePath);
    std::filesystem::rename(updatePath, executablePath);
    LOGFILE << "[I] Replaced " << executablePath << " with " << updatePath << std::endl;
  } catch (const std::filesystem::filesystem_error& e) {
    LOGFILE << "[E] Failed to replace executable post-update: " << e.what() << std::endl;
    return;
  }
  // Ensure new file is executable
  try {
    std::filesystem::permissions(executablePath,
      std::filesystem::perms::owner_exec |
      std::filesystem::perms::group_exec |
      std::filesystem::perms::others_exec,
      std::filesystem::perm_options::add);
    LOGFILE << "[I] Set file permissions for " << executablePath << " to executable" << std::endl;
  } catch (const std::filesystem::filesystem_error& e) {
    LOGFILE << "[W] Failed to set file permissions for " << executablePath << ": " << e.what() << std::endl;
  }
  QMessageBox::information(nullptr, "Spplice Update",
    "Spplice has been updated.\nRestart Spplice to apply the changes.");
#endif

}

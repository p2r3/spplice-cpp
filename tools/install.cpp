#include <iostream>
#include <filesystem>
#include <fstream>
#include <string>
#include <sstream>
#include <unistd.h>
#include <dirent.h>
#include <cstdlib>
#include <thread>
#include <chrono>
#include <archive.h>
#include <archive_entry.h>
#include "../globals.h" // Project globals
#include "curl.h" // ToolsCURL

// Definitions for this source file
#include "install.h"

// Extracts a tar.xz archive
bool ToolsInstall::extractLocalFile (const std::filesystem::path path, const std::filesystem::path dest) {

  struct archive* archive;
  struct archive* extracted;
  struct archive_entry* entry;
  int r;

  archive = archive_read_new();
  archive_read_support_format_tar(archive);
  archive_read_support_filter_xz(archive);
  extracted = archive_write_disk_new();
  archive_write_disk_set_options(extracted, ARCHIVE_EXTRACT_TIME);

  if ((r = archive_read_open_filename(archive, path.c_str(), 10240))) {
    std::cerr << "Could not open file: " << archive_error_string(archive) << std::endl;
    return false;
  }

  bool output = true;
  while (archive_read_next_header(archive, &entry) == ARCHIVE_OK) {

    std::filesystem::path full_path = dest / archive_entry_pathname(entry);
    archive_entry_set_pathname(entry, full_path.c_str());

    std::cout << "Extracting: " << archive_entry_pathname(entry) << std::endl;
    archive_write_header(extracted, entry);

    const void* buff;
    size_t size;
    la_int64_t offset;

    while (true) {
      r = archive_read_data_block(archive, &buff, &size, &offset);
      if (r == ARCHIVE_EOF) break;
      if (r != ARCHIVE_OK) {
        std::cerr << "Archive read error: " << archive_error_string(archive) << std::endl;
        output = false;
        break;
      }
      r = archive_write_data_block(extracted, buff, size, offset);
      if (r != ARCHIVE_OK) {
        std::cerr << "Archive write error: " << archive_error_string(extracted) << std::endl;
        output = false;
        break;
      }
    }
    archive_write_finish_entry(extracted);

  }

  archive_read_close(archive);
  archive_read_free(archive);
  archive_write_close(extracted);
  archive_write_free(extracted);

  return output;

}

// Retrieves the path to a process executable using its name
std::string getProcessPath (const std::string &processName) {

  std::string executablePath = "";

  DIR *dir = opendir("/proc");
  if (!dir) return executablePath;

  // Look through /proc to find the PID of the given process
  struct dirent *entry;
  while ((entry = readdir(dir)) != NULL) {

    // If not a PID, continue
    if (!isdigit(*entry->d_name)) continue;

    // Get the command line of the current PID
    std::string cmdlineLink = "/proc/" + std::string(entry->d_name) + "/cmdline";
    std::ifstream cmdlineFile(cmdlineLink);
    if (!cmdlineFile.is_open()) continue;

    std::string cmdline;
    std::getline(cmdlineFile, cmdline);

    // If the process is not mentioned in this command line, continue searching
    std::size_t index = cmdline.find(processName);
    if (index == std::string::npos) continue;

    // Make sure that what we've found is a binary, not a path or argument
    // Since we're searching for a path, it's safe to assume the binary is prefixed with a slash
    const char prefix = cmdline[index - 1];
    const char suffix = cmdline[index + processName.length()];
    if (prefix != '/' || (suffix != ' ' && suffix != '\0')) continue;

    // Use the PID we obtained to get the executable path
    char exePath[1024] = {0};
    std::string exeLink = "/proc/" + std::string(entry->d_name) + "/exe";

    ssize_t len = readlink(exeLink.c_str(), exePath, sizeof(exePath) - 1);
    if (len == -1) continue;
    executablePath = exePath;

  }
  closedir(dir);

  return executablePath;

}

// Finds the Steam binary and uses it to start Portal 2
bool startPortal2 () {

  std::string steamPath = getProcessPath("steam");
  if (steamPath == "") {
    std::cerr << "Failed to find Steam process path. Is Steam running?" << std::endl;
    return false;
  }

  pid_t pid = fork();
  if (pid == -1) {
    std::cerr << "Failed to fork process." << std::endl;
    return false;
  }

  // This applies to the child process from fork()
  if (pid == 0) {
    execl(steamPath.c_str(), steamPath.c_str(), "-applaunch", "620", "-tempcontent", NULL);

    // execl only returns on error
    std::cerr << "Failed to call Steam binary from fork." << std::endl;
    _exit(1);
  }

  return true;

}

// Creates a symbolic link for a directory on Linux, and an NTFS junction on Windows
bool linkDirectory (const std::filesystem::path target, const std::filesystem::path linkName) {

  if (symlink(target.c_str(), linkName.c_str()) != 0) {
    std::cerr << "Failed to create symbolic link " << target << " -> " << linkName << std::endl;
    return false;
  }
  return true;

}

// Creates a hard link for a file on both Linux and Windows
bool linkFile (const std::filesystem::path target, const std::filesystem::path linkName) {

  if (link(target.c_str(), linkName.c_str()) != 0) {
    std::cerr << "Failed to create hard link " << target << " -> " << linkName << std::endl;
    return false;
  }
  return true;

}

// Removes a symbolic link to a directory on Linux, or an NTFS junction on Windows
bool unlinkDirectory (const std::filesystem::path target) {

  if (unlink(target.c_str()) != 0) {
    std::cerr << "Failed to remove symbolic link " << target << std::endl;
    return false;
  }
  return true;

}

// Installs the package located at the temporary directory (TEMP_DIR/current_package)
void ToolsInstall::installTempFile (std::function<void(const std::string)> failCallback, std::function<void()> successCallback) {

  // Extract the package to a temporary directory
  const std::filesystem::path tmpPackageFile = TEMP_DIR / "current_package";
  const std::filesystem::path tmpPackageDirectory = TEMP_DIR / "tempcontent";

  std::filesystem::create_directories(tmpPackageDirectory);

  if (!extractLocalFile(tmpPackageFile, tmpPackageDirectory)) {
    return failCallback("Failed to extract package.");
  }
  std::filesystem::remove(tmpPackageFile);

  // Ensure the existence of a soundcache directory
  std::filesystem::create_directories(tmpPackageDirectory / "maps");
  std::filesystem::create_directories(tmpPackageDirectory / "maps" / "soundcache");

  // Start Portal 2
  if (!startPortal2()) {
    return failCallback("Failed to start Portal 2. Is Steam running?");
  }

  // Find the Portal 2 game files path
  std::string gameProcessPath = "";
  while (gameProcessPath == "") {
    gameProcessPath = getProcessPath("portal2_linux");
  }
  std::filesystem::path gamePath = std::filesystem::path(gameProcessPath).parent_path();
  std::cout << "Found Portal 2 at " << gameProcessPath << std::endl;

  // Link the extracted package files to the destination tempcontent directory
  std::filesystem::path tempcontentPath = gamePath / "portal2_tempcontent";
  if (!linkDirectory(tmpPackageDirectory, tempcontentPath)) {
    return failCallback("Failed to link package files to portal2_tempcontent.");
  }

  // Link the soundcache from base Portal 2 to skip waiting for it to generate
  linkFile(gamePath / "portal2" / "maps" / "soundcache" / "_master.cache", tempcontentPath / "maps" / "soundcache" / "_master.cache");

  // Report install success to the UI
  successCallback();

  // Stall until Portal 2 has been closed
  while (getProcessPath("portal2_linux") != "") {
    std::this_thread::sleep_for(std::chrono::milliseconds(200));
  }

  // When Portal 2 exits, remove the tempcontent link and all related files
  unlinkDirectory(tempcontentPath);
  std::filesystem::remove_all(tmpPackageDirectory);
  std::cout << "Unlinked and deleted package files" << std::endl;

}

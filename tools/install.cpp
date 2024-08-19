#include <iostream>
#include <utility>
#include <filesystem>
#include <fstream>
#include <string>
#include <sstream>
#include <dirent.h>
#include <cstdlib>
#include <thread>
#include <chrono>
#include <archive.h>
#include <archive_entry.h>
#include "../globals.h" // Project globals
#include "curl.h" // ToolsCURL
#include "qt.h" // ToolsQT

#ifdef TARGET_WINDOWS
#include <windows.h>
#include <tlhelp32.h>
#include <psapi.h>
#else
#include <unistd.h>
#endif

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

  if ((r = archive_read_open_filename_w(archive, path.wstring().c_str(), 10240))) {
    std::cerr << "Could not open file: " << archive_error_string(archive) << std::endl;
    return false;
  }

  bool output = true;
  while (archive_read_next_header(archive, &entry) == ARCHIVE_OK) {

    std::filesystem::path full_path = dest / archive_entry_pathname(entry);
    archive_entry_set_pathname_utf8(entry, full_path.string().c_str());

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
#ifndef TARGET_WINDOWS
std::wstring ToolsInstall::getProcessPath (const std::string &processName) {

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
#else
std::wstring ToolsInstall::getProcessPath (const std::string &processName) {

  HANDLE snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
  if (snapshot == INVALID_HANDLE_VALUE) {
    return L"";
  }

  PROCESSENTRY32 processEntry;
  processEntry.dwSize = sizeof(PROCESSENTRY32);

  if (!Process32First(snapshot, &processEntry)) {
    CloseHandle(snapshot);
    return L"";
  }

  do {
    if (processName == processEntry.szExeFile) {
      wchar_t path[MAX_PATH];
      HANDLE process = OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, FALSE, processEntry.th32ProcessID);
      if (process) {
        if (GetModuleFileNameExW(process, nullptr, path, MAX_PATH)) {
          CloseHandle(process);
          CloseHandle(snapshot);
          return std::wstring(path);
        }
        CloseHandle(process);
      }
    }
  } while (Process32Next(snapshot, &processEntry));

  CloseHandle(snapshot);
  return L"";

}
#endif

// Finds the Steam binary and uses it to start Portal 2
#ifndef TARGET_WINDOWS
bool startPortal2 () {

  std::wstring steamPath = ToolsInstall::getProcessPath("steam");
  if (steamPath.length() == 0) {
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
    execl(steamPath.c_str(), steamPath.c_str(), "-applaunch", "620", "-tempcontent", "+alias", "cmd1", NULL);

    // execl only returns on error
    std::cerr << "Failed to call Steam binary from fork." << std::endl;
    _exit(1);
  }

  return true;

}
#else
bool startPortal2 () {

  std::wstring steamPath = ToolsInstall::getProcessPath("steam.exe");
  if (steamPath.length() == 0) {
    std::cerr << "Failed to find Steam process path. Is Steam running?" << std::endl;
    return false;
  }

  // Define the structures used in CreateProcess
  STARTUPINFOW si = {0};
  PROCESS_INFORMATION pi = {0};

  // Initialize the STARTUPINFO structure
  si.cb = sizeof(si);
  si.dwFlags = STARTF_USESHOWWINDOW;
  si.wShowWindow = SW_HIDE;  // Hide the window for the detached process

  std::wstring args = L"-applaunch 620 -tempcontent +alias cmd1";

  // Create the process
  if (!CreateProcessW(
    steamPath.c_str(),       // Path to the executable
    const_cast<LPWSTR>(args.c_str()), // Command line arguments
    nullptr,               // Process handle not inheritable
    nullptr,               // Thread handle not inheritable
    FALSE,                 // Set handle inheritance to FALSE
    CREATE_NEW_CONSOLE,    // Create a new console for the process
    nullptr,               // Use parent's environment block
    nullptr,               // Use parent's starting directory
    &si,                   // Pointer to STARTUPINFO structure
    &pi                    // Pointer to PROCESS_INFORMATION structure
  )) {
    std::wcerr << L"CreateProcessW failed (" << GetLastError() << L")\n";
    return false;
  }

  // Close process and thread handles (we don't need to wait for the process)
  CloseHandle(pi.hProcess);
  CloseHandle(pi.hThread);

  return true;

}
#endif

// Creates a symbolic link for a directory on Linux, and an NTFS junction on Windows
#ifndef TARGET_WINDOWS
bool linkDirectory (const std::filesystem::path target, const std::filesystem::path linkName) {

  if (symlink(target.c_str(), linkName.c_str()) != 0) {
    std::cerr << "Failed to create symbolic link " << target << " -> " << linkName << std::endl;
    return false;
  }
  return true;

}
#else
bool linkDirectory (const std::filesystem::path target, const std::filesystem::path linkName) {

  std::string command = "mklink /J \"" + linkName.string() + "\" \"" + target.string() + "\"";
  int result = std::system(command.c_str());

  if (result != 0) {
    std::cerr << "Failed to create junction " << target << " -> " << linkName << ": exit code " << result << std::endl;
    return false;
  }
  return true;

}
#endif

// Creates a hard link for a file on both Linux and Windows
#ifndef TARGET_WINDOWS
bool linkFile (const std::filesystem::path target, const std::filesystem::path linkName) {

  if (link(target.c_str(), linkName.c_str()) != 0) {
    std::cerr << "Failed to create hard link " << target << " -> " << linkName << std::endl;
    return false;
  }
  return true;

}
#else
bool linkFile (const std::filesystem::path target, const std::filesystem::path linkName) {

  // Convert paths to wide strings
  std::wstring targetWStr = target.wstring();
  std::wstring linkNameWStr = linkName.wstring();

  // Use CreateHardLinkW to create the hard link
  BOOL result = CreateHardLinkW(linkNameWStr.c_str(), targetWStr.c_str(), NULL);

  if (result == 0) {
    std::cerr << "Failed to create hard link " << target << " -> " << linkName << ": " << GetLastError() << std::endl;
    return false;
  }
  return true;

}
#endif

// Removes a symbolic link to a directory on Linux, or an NTFS junction on Windows
#ifndef TARGET_WINDOWS
bool unlinkDirectory (const std::filesystem::path target) {

  if (unlink(target.c_str()) != 0) {
    std::cerr << "Failed to remove symbolic link " << target << std::endl;
    return false;
  }
  return true;

}
#else
bool unlinkDirectory (const std::filesystem::path target) {

  // Check if the target exists and is a directory
  if (!std::filesystem::exists(target) || !std::filesystem::is_directory(target)) {
    return false;
  }

  // Try to delete the directory
  BOOL success = RemoveDirectoryW(target.wstring().c_str());

  // Check if RemoveDirectory succeeded
  if (success) {
    return true;
  } else {
    std::cerr << "Failed to remove junction " << target << ": " << GetLastError() << std::endl;
    return false;
  }

}
#endif

// Installs the package located at the temporary directory (TEMP_DIR/current_package)
std::pair<bool, std::wstring> ToolsInstall::installTempFile () {

  // Extract the package to a temporary directory
  const std::filesystem::path tmpPackageFile = TEMP_DIR / "current_package";
  const std::filesystem::path tmpPackageDirectory = TEMP_DIR / "tempcontent";

  std::filesystem::create_directories(tmpPackageDirectory);

  if (!extractLocalFile(tmpPackageFile, tmpPackageDirectory)) {
    return std::pair<bool, std::wstring> (false, L"Failed to extract package.");
  }
  std::filesystem::remove(tmpPackageFile);

  // Ensure the existence of a soundcache directory
  std::filesystem::create_directories(tmpPackageDirectory / "maps");
  std::filesystem::create_directories(tmpPackageDirectory / "maps" / "soundcache");

  // Start Portal 2
  if (!startPortal2()) {
    return std::pair<bool, std::wstring> (false, L"Failed to start Portal 2. Is Steam running?");
  }

  // Find the Portal 2 game files path
  std::wstring gameProcessPath = L"";
  while (gameProcessPath.length() == 0) {
#ifndef TARGET_WINDOWS
    gameProcessPath = ToolsInstall::getProcessPath("portal2_linux");
#else
    gameProcessPath = ToolsInstall::getProcessPath("portal2.exe");
#endif
  }
  std::filesystem::path gamePath = std::filesystem::path(gameProcessPath).parent_path();
  std::cout << "Found Portal 2 at " << gamePath << std::endl;

  // Link the extracted package files to the destination tempcontent directory
  std::filesystem::path tempcontentPath = gamePath / "portal2_tempcontent";
  if (!linkDirectory(tmpPackageDirectory, tempcontentPath)) {
    return std::pair<bool, std::wstring> (false, L"Failed to link package files to portal2_tempcontent.");
  }

  // Link the soundcache from base Portal 2 to skip waiting for it to generate
  linkFile(gamePath / "portal2" / "maps" / "soundcache" / "_master.cache", tempcontentPath / "maps" / "soundcache" / "_master.cache");

  // Report install success to the UI
  return std::pair<bool, std::wstring> (true, gamePath.wstring());

}

void ToolsInstall::Uninstall (const std::wstring &gamePath) {

  // Remove the tempcontent directory link
  const std::filesystem::path tempcontentPath = std::filesystem::path(gamePath) / "portal2_tempcontent";
  unlinkDirectory(tempcontentPath);

  // Remove the actual package directory containing all package files
  const std::filesystem::path tmpPackageDirectory = TEMP_DIR / "tempcontent";
  std::filesystem::remove_all(tmpPackageDirectory);

}

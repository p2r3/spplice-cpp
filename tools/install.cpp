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

#include "../globals.h" // Project globals
#include "curl.h" // ToolsCURL
#include "netcon.h" // ToolsNetCon
#include "qt.h" // ToolsQT
#include "js.h" // ToolsJS

#ifdef TARGET_WINDOWS
  #include "../deps/win32/include/archive.h"
  #include "../deps/win32/include/archive_entry.h"

  #include <windows.h>
  #include <tlhelp32.h>
  #include <psapi.h>
  #include <locale>
  #include <codecvt>
#else
  #include <archive.h>
  #include <archive_entry.h>

  #include <sys/stat.h>
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
    LOGFILE << "[E] Could not open file: " << archive_error_string(archive) << std::endl;
    return false;
  }

  bool output = true;
  while (archive_read_next_header(archive, &entry) == ARCHIVE_OK) {

    std::filesystem::path full_path = dest / archive_entry_pathname(entry);
    archive_entry_set_pathname_utf8(entry, full_path.string().c_str());

    LOGFILE << "[I] Extracting: " << '"' << archive_entry_pathname(entry) << '"' << std::endl;
    archive_write_header(extracted, entry);

    const void* buff;
    size_t size;
    la_int64_t offset;

    while (true) {
      r = archive_read_data_block(archive, &buff, &size, &offset);
      if (r == ARCHIVE_EOF) break;
      if (r != ARCHIVE_OK) {
        LOGFILE << "[E] Archive read error: " << archive_error_string(archive) << std::endl;
        output = false;
        break;
      }
      r = archive_write_data_block(extracted, buff, size, offset);
      if (r != ARCHIVE_OK) {
        LOGFILE << "[E] Archive write error: " << archive_error_string(extracted) << std::endl;
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
std::string ToolsInstall::getProcessPath (const std::string &processName) {

  DIR *dir = opendir("/proc");
  if (!dir) return "";

  // Since we're looking for a path, it's safe to assume that the binary is prefixed with a slash
  std::string queryString = "/" + processName;
  std::size_t queryLength = queryString.length();

  // Look through /proc to find the command line which launched the given process
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
    cmdlineFile.close();

    // Get the index of the end of the path in the current command line
    std::size_t pathEnd = std::min(cmdline.find('\0'), cmdline.length());
    // Check if the command line path is shorter than our query string
    if (pathEnd < queryLength) continue;

    // Check if the command line path ends with the string we're looking for
    if (cmdline.substr(pathEnd - queryLength, queryLength) == queryString) {
      closedir(dir);
      return cmdline.substr(0, pathEnd);
    }

  }

  // If we haven't returned by now, we found nothing
  closedir(dir);
  return "";

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
bool startPortal2 (const std::vector<std::string> extraArgs) {

  std::string steamPath = ToolsInstall::getProcessPath("steam");
  if (steamPath.length() == 0) {
    LOGFILE << "[E] Failed to find Steam process path. Is Steam running?" << std::endl;
    return false;
  }

  pid_t pid = fork();
  if (pid == -1) {
    LOGFILE << "[E] Failed to fork process." << std::endl;
    return false;
  }

  // This applies to the child process from fork()
  if (pid == 0) {

    std::string appid = std::to_string(SPPLICE_STEAMAPP_APPIDS[SPPLICE_STEAMAPP_INDEX]);

    // Create a vector of arguments for the Steam call
    std::vector<const char*> args;
    args.push_back(steamPath.c_str());
    args.push_back("-applaunch");
    args.push_back(appid.c_str());
    args.push_back("-tempcontent");

    // If a valid port has been established, enable TCP console server
    if (SPPLICE_NETCON_PORT != -1) {
      args.push_back("-netconport");
      args.push_back(std::to_string(SPPLICE_NETCON_PORT).c_str());
    }

    // Append any additional package-specific arguments
    for (const std::string &arg : extraArgs) {
      args.push_back(arg.c_str());
    }

    args.push_back(nullptr);
    execv(steamPath.c_str(), const_cast<char* const*>(args.data()));

    // execv only returns on error
    LOGFILE << "[E] Failed to call Steam binary from fork." << std::endl;

    // If the above failed, revert to using the Steam browser protocol
    std::string command = "xdg-open steam://run/"+ appid +"//-tempcontent";
    if (SPPLICE_NETCON_PORT != -1) command += " -netconport " + std::to_string(SPPLICE_NETCON_PORT);

    for (const std::string &arg : extraArgs) {
      command += " " + arg;
    }

    if (system(command.c_str()) != 0) {
      LOGFILE << "[E] Failed to open Steam URI." << std::endl;
    }

    // Exit from the child process
    _exit(1);

  }

  return true;

}
#else
bool startPortal2 (const std::vector<std::string> extraArgs) {

  std::wstring steamPath = ToolsInstall::getProcessPath("steam.exe");
  if (steamPath.length() == 0) {
    LOGFILE << "[E] Failed to find Steam process path. Is Steam running?" << std::endl;
    return false;
  }

  // Define the structures used in CreateProcess
  STARTUPINFOW si = {0};
  PROCESS_INFORMATION pi = {0};

  // Initialize the STARTUPINFO structure
  si.cb = sizeof(si);
  si.dwFlags = STARTF_USESHOWWINDOW;
  si.wShowWindow = SW_HIDE;  // Hide the window for the detached process

  std::wstring appid = std::to_wstring(SPPLICE_STEAMAPP_APPIDS[SPPLICE_STEAMAPP_INDEX]);
  std::wstring args = L"-applaunch " + appid + L" -tempcontent -condebug";
  if (SPPLICE_NETCON_PORT != -1) {
    args += L" -netconport " + std::to_wstring(SPPLICE_NETCON_PORT);
  }

  // Append any additional package-specific arguments
  for (const std::string &arg : extraArgs) {
    std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>> converter;
    std::wstring warg = converter.from_bytes(arg);
    args += L" " + warg;
  }

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
    LOGFILE << "[E] Failed to create symbolic link " << target << " -> " << linkName << std::endl;
    return false;
  }
  return true;

}
#else
#define REPARSE_MOUNTPOINT_HEADER_SIZE 8
typedef struct {
  DWORD ReparseTag;
  DWORD ReparseDataLength;
  WORD Reserved;
  WORD ReparseTargetLength;
  WORD ReparseTargetMaximumLength;
  WORD Reserved1;
  WCHAR ReparseTarget[1];
} REPARSE_MOUNTPOINT_DATA_BUFFER, *PREPARSE_MOUNTPOINT_DATA_BUFFER;

// Taken from: http://www.flexhex.com/docs/articles/hard-links.phtml
// Modified for error handling and wstring support
bool linkDirectory (const std::filesystem::path target, const std::filesystem::path linkName) {

  LPCWSTR szJunction = linkName.c_str();
  LPCWSTR szPath = target.c_str();

  BYTE buf[sizeof(REPARSE_MOUNTPOINT_DATA_BUFFER) + MAX_PATH * sizeof(WCHAR)];
  REPARSE_MOUNTPOINT_DATA_BUFFER& ReparseBuffer = (REPARSE_MOUNTPOINT_DATA_BUFFER&)buf;
  wchar_t szTarget[MAX_PATH] = L"\\??\\";

  wcscat(szTarget, szPath);
  wcscat(szTarget, L"\\");

  if (!CreateDirectoryW(szJunction, NULL)) {
    LOGFILE << "[E] Failed to create directory for junction " << linkName << ": ERRNO " << GetLastError() << std::endl;
    return false;
  }

  // Obtain SE_RESTORE_NAME privilege (required for opening a directory)
  HANDLE hToken = NULL;
  TOKEN_PRIVILEGES tp;
  if (!OpenProcessToken(GetCurrentProcess(), TOKEN_ADJUST_PRIVILEGES, &hToken)) {
    LOGFILE << "[E] Failed to open process token: " << GetLastError() << std::endl;
    return false;
  }
  if (!LookupPrivilegeValue(NULL, SE_RESTORE_NAME, &tp.Privileges[0].Luid)) {
    LOGFILE << "[E] Failed to look up SE_RESTORE_NAME privilege: " << GetLastError() << std::endl;
    return false;
  }
  tp.PrivilegeCount = 1;
  tp.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;
  if (!AdjustTokenPrivileges(hToken, FALSE, &tp, sizeof(TOKEN_PRIVILEGES), NULL, NULL)) {
    LOGFILE << "[E] Failed to adjust process privileges: " << GetLastError() << std::endl;
    return false;
  }
  if (hToken) CloseHandle(hToken);

  HANDLE hDir = CreateFileW(szJunction, GENERIC_WRITE, 0, NULL, OPEN_EXISTING, FILE_FLAG_OPEN_REPARSE_POINT | FILE_FLAG_BACKUP_SEMANTICS, NULL);
  if (hDir == INVALID_HANDLE_VALUE) {
    LOGFILE << "[E] Failed to create junction file \"" << linkName << "\": ERRNO " << GetLastError() << std::endl;
    return false;
  }

  memset(buf, 0, sizeof(buf));
  ReparseBuffer.ReparseTag = IO_REPARSE_TAG_MOUNT_POINT;

  wcsncpy(ReparseBuffer.ReparseTarget, szTarget, MAX_PATH);
  ReparseBuffer.ReparseTarget[MAX_PATH - 1] = L'\0';
  int len = wcslen(ReparseBuffer.ReparseTarget) + 1;

  ReparseBuffer.ReparseTargetMaximumLength = (len--) * sizeof(WCHAR);
  ReparseBuffer.ReparseTargetLength = len * sizeof(WCHAR);
  ReparseBuffer.ReparseDataLength = ReparseBuffer.ReparseTargetLength + 12;

  DWORD dwRet;
  if (!DeviceIoControl(hDir, FSCTL_SET_REPARSE_POINT, &ReparseBuffer, ReparseBuffer.ReparseDataLength+REPARSE_MOUNTPOINT_HEADER_SIZE, NULL, 0, &dwRet, NULL)) {
    DWORD dr = GetLastError();
    CloseHandle(hDir);
    RemoveDirectoryW(szJunction);

    LOGFILE << "[E] Failed to create reparse point " << target << " -> " << linkName << ": ERRNO " << dr << std::endl;
    return false;
  }

  CloseHandle(hDir);
  return true;

}
#endif

// Creates a symbolic link for a file on Linux and a hard link on Windows
#ifndef TARGET_WINDOWS
bool linkFile (const std::filesystem::path target, const std::filesystem::path linkName) {

  if (symlink(target.c_str(), linkName.c_str()) != 0) {
    LOGFILE << "[E] Failed to create symbolic link " << target << " -> " << linkName << std::endl;
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
    LOGFILE << "[E] Failed to create hard link " << target << " -> " << linkName << ": ERRNO " << GetLastError() << std::endl;
    return false;
  }
  return true;

}
#endif

// Removes a symbolic link to a directory on Linux, or an NTFS junction on Windows
#ifndef TARGET_WINDOWS
bool unlinkDirectory (const std::filesystem::path target) {

  if (unlink(target.c_str()) != 0) {
    LOGFILE << "[E] Failed to remove symbolic link " << target << std::endl;
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
    LOGFILE << "[E] Failed to remove junction " << target << ": ERRNO " << GetLastError() << std::endl;
    return false;
  }

}
#endif

#ifndef TARGET_WINDOWS
bool isDirectoryLink (const std::filesystem::path linkName) {

  struct stat path_stat;
  if (lstat(linkName.c_str(), &path_stat) != 0) {
    LOGFILE << "[E] Failed to stat path " << linkName << std::endl;
    return false;
  }
  return S_ISLNK(path_stat.st_mode);

}
#else
bool isDirectoryLink (const std::filesystem::path linkName) {

  DWORD attributes = GetFileAttributesW(linkName.wstring().c_str());

  if (attributes == INVALID_FILE_ATTRIBUTES) {
    LOGFILE << "[E] Failed to get file attributes for " << linkName << std::endl;
    return false;
  }

  if (!(attributes & FILE_ATTRIBUTE_REPARSE_POINT)) {
    return false;
  }
  return true;

}
#endif

// Installs the package located at the temporary directory (CACHE_DIR/current_package)
std::string ToolsInstall::installPackageFile (const std::filesystem::path packageFile, const std::vector<std::string> args) {

  // Extract the package to a temporary directory
  const std::filesystem::path tmpPackageDirectory = CACHE_DIR / "tempcontent";
  if (std::filesystem::exists(tmpPackageDirectory)) {
    std::filesystem::remove_all(tmpPackageDirectory);
  }
  std::filesystem::create_directories(tmpPackageDirectory);

  if (!extractLocalFile(packageFile, tmpPackageDirectory)) {
    return "Failed to extract package. Please clear the cache and try again.";
  }

  // Ensure the existence of a soundcache directory
  std::filesystem::create_directories(tmpPackageDirectory / "maps");
  std::filesystem::create_directories(tmpPackageDirectory / "maps" / "soundcache");

  // Find the JS API entry point
  const std::filesystem::path jsEntryPoint = tmpPackageDirectory / "main.js";
  bool jsEntryPointExists = std::filesystem::exists(jsEntryPoint);

  // Disable the TCP console server by default
  SPPLICE_NETCON_PORT = -1;

  // If the JS API is in use, enable the TCP console server
  if (jsEntryPointExists) {
    // Use the default Source listen port, which is very likely to be free
    // In Portal 2, only a UDP connection exists on this port by default
    SPPLICE_NETCON_PORT = 27015;
  }

  // Start Portal 2
  if (!startPortal2(args)) {
    std::filesystem::remove_all(tmpPackageDirectory);
    return "Failed to start Portal 2. Is Steam running?";
  }

  // Find the Portal 2 game files path
#ifndef TARGET_WINDOWS
  std::string gameProcessPath = "";
  while (gameProcessPath.length() == 0) {
    gameProcessPath = ToolsInstall::getProcessPath("portal2_linux");
  }
#else
  std::wstring gameProcessPath = L"";
  while (gameProcessPath.length() == 0) {
    gameProcessPath = ToolsInstall::getProcessPath("portal2.exe");
  }
#endif

  GAME_DIR = std::filesystem::path(gameProcessPath).parent_path();
  LOGFILE << "[I] Found Portal 2 at " << GAME_DIR << std::endl;

  std::filesystem::path tempcontentPath = GAME_DIR / "portal2_tempcontent";

  // Handle an existing tempcontent directory
  if (std::filesystem::exists(tempcontentPath)) {
    if (isDirectoryLink(tempcontentPath)) {
      // If it's a symlink or junction, just remove it
      unlinkDirectory(tempcontentPath);
    } else {
      // If it's an actual file/directory, move it away to be safe
      std::filesystem::path tempcontentBackupPath = GAME_DIR.parent_path() / ".spplice_tempcontent_backup";
      std::filesystem::create_directories(tempcontentBackupPath);
      // Check for the next available backup slot
      for (int i = 1; i <= 64; i ++) {
        std::filesystem::path newPath = tempcontentBackupPath / ("portal2_tempcontent_" + std::to_string(i));
        if (std::filesystem::exists(newPath)) continue;
        std::filesystem::rename(tempcontentPath, newPath);
        break;
      }
    }
  }

  // Link the extracted package files to the destination tempcontent directory
  if (!linkDirectory(tmpPackageDirectory, tempcontentPath)) {
    std::filesystem::remove_all(tmpPackageDirectory);
    return "Failed to link package files to portal2_tempcontent.";
  }
  LOGFILE << "[I] Linked " << tmpPackageDirectory << " to " << tempcontentPath << std::endl;

  const std::filesystem::path soundcacheSourcePath = GAME_DIR / "portal2" / "maps" / "soundcache" / "_master.cache";
  const std::filesystem::path soundcacheDestPath = tempcontentPath / "maps" / "soundcache" / "_master.cache";

  // Link the soundcache from base Portal 2 to skip waiting for it to generate
  if (!linkFile(soundcacheSourcePath, soundcacheDestPath)) {
    // If linking the soundcache failed, copy it instead
    LOGFILE << "[W] Failed to link soundcache file, copying instead" << std::endl;
    std::filesystem::copy_file(soundcacheSourcePath, soundcacheDestPath);
  } else {
    LOGFILE << "[I] Linked soundcache file" << std::endl;
  }

  // Run the JS API entrypoint (if one exists) on a detached thread
  if (jsEntryPointExists) {
    LOGFILE << "[I] Running JavaScript API entrypoint file" << std::endl;
    std::thread ([jsEntryPoint]() {
      try {
        ToolsJS::runFile(jsEntryPoint);
      } catch (const std::exception &e) {
        LOGFILE << "[E] Fatal JS API exception: " << std::string(e.what()) << std::endl;
      }
    }).detach();
  }

  // Report install success to the UI
  LOGFILE << "[I] Installation of " << packageFile << " completed" << std::endl;
  return "";

}

// Kills any running Portal 2 process if install state is non-zero
bool ToolsInstall::killPortal2 () {

  // If no package is installed, do nothing
  if (SPPLICE_INSTALL_STATE == 0) return false;

  // Use a platform-specific command string for killing the process
#ifndef TARGET_WINDOWS
  const std::string command = "pkill -e portal2_linux -9";
#else
  const std::string command = "taskkill /F /T /IM portal2.exe";
#endif

  int result = std::system(command.c_str());
  if (result != 0) {
    LOGFILE << "[E] Failed to kill Portal 2. Error code: " << result << std::endl;
    return false;
  } else {
    LOGFILE << "[I] Killed Portal 2." << std::endl;
    return true;
  }

}

void ToolsInstall::uninstall () {

  // If no package is installed, do nothing
  if (GAME_DIR.empty() || SPPLICE_INSTALL_STATE == 0) return;

  // Remove the tempcontent directory link
  const std::filesystem::path tempcontentPath = GAME_DIR / "portal2_tempcontent";
  if (isDirectoryLink(tempcontentPath)) unlinkDirectory(tempcontentPath);

  // Remove the actual package directory containing all package files
  const std::filesystem::path tmpPackageDirectory = CACHE_DIR / "tempcontent";
  std::filesystem::remove_all(tmpPackageDirectory);

  // HACK: The Windows console workaround creates a logfile that needs to be cleaned up
#ifdef TARGET_WINDOWS
  std::filesystem::path logPath = (GAME_DIR / "portal2") / "console.log";
  if (std::filesystem::exists(logPath)) std::filesystem::remove(logPath);
#endif

}

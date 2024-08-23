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
#include "js.h" // ToolsJS

#ifdef TARGET_WINDOWS
  #include <windows.h>
  #include <tlhelp32.h>
  #include <psapi.h>
  #include <locale>
  #include <codecvt>
#else
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

    // Create a vector of arguments for the Steam call
    std::vector<const char*> args;
    args.push_back(steamPath.c_str());
    args.push_back("-applaunch");
    args.push_back("620");
    args.push_back("-tempcontent");
    args.push_back("-netconport");
    args.push_back(std::to_string(SPPLICE_NETCON_PORT).c_str());

    // Append any additional package-specific arguments
    for (const std::string &arg : extraArgs) {
      args.push_back(arg.c_str());
    }

    args.push_back(nullptr);
    execv(steamPath.c_str(), const_cast<char* const*>(args.data()));

    // execv only returns on error
    std::cerr << "Failed to call Steam binary from fork." << std::endl;

    // If the above failed, revert to using the Steam browser protocol
    std::string command = "xdg-open steam://run/620//-tempcontent -netconport " + SPPLICE_NETCON_PORT;
    for (const std::string &arg : extraArgs) {
      command += " " + arg;
    }

    if (system(command.c_str()) != 0) {
      std::cerr << "Failed to open Steam URI." << std::endl;
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

  std::wstring args = L"-applaunch 620 -tempcontent -netconport " + SPPLICE_NETCON_PORT;

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
    std::cerr << "Failed to create symbolic link " << target << " -> " << linkName << std::endl;
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
    std::cerr << "Failed to create directory for junction " << linkName << ": " << GetLastError() << std::endl;
    return false;
  }

  // Obtain SE_RESTORE_NAME privilege (required for opening a directory)
  HANDLE hToken = NULL;
  TOKEN_PRIVILEGES tp;
  if (!OpenProcessToken(GetCurrentProcess(), TOKEN_ADJUST_PRIVILEGES, &hToken)) {
    std::cerr << "Failed to open process token: " << GetLastError() << std::endl;
    return false;
  }
  if (!LookupPrivilegeValue(NULL, SE_RESTORE_NAME, &tp.Privileges[0].Luid)) {
    std::cerr << "Failed to look up SE_RESTORE_NAME privilege: " << GetLastError() << std::endl;
    return false;
  }
  tp.PrivilegeCount = 1;
  tp.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;
  if (!AdjustTokenPrivileges(hToken, FALSE, &tp, sizeof(TOKEN_PRIVILEGES), NULL, NULL)) {
    std::cerr << "Failed to adjust process privileges: " << GetLastError() << std::endl;
    return false;
  }
  if (hToken) CloseHandle(hToken);

  HANDLE hDir = CreateFileW(szJunction, GENERIC_WRITE, 0, NULL, OPEN_EXISTING, FILE_FLAG_OPEN_REPARSE_POINT | FILE_FLAG_BACKUP_SEMANTICS, NULL);
  if (hDir == INVALID_HANDLE_VALUE) {
    std::cerr << "Failed to create junction file " << linkName << ": " << GetLastError() << std::endl;
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

    std::cerr << "Failed to create reparse point " << target << " -> " << linkName << ": " << dr << std::endl;
    return false;
  }

  CloseHandle(hDir);
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

#ifndef TARGET_WINDOWS
bool isDirectoryLink (const std::filesystem::path linkName) {

  struct stat path_stat;
  if (lstat(linkName.c_str(), &path_stat) != 0) {
    std::cerr << "Failed to stat path " << linkName << std::endl;
    return false;
  }
  return S_ISLNK(path_stat.st_mode);

}
#else
bool isDirectoryLink (const std::filesystem::path linkName) {

  DWORD attributes = GetFileAttributesW(linkName.wstring().c_str());

  if (attributes == INVALID_FILE_ATTRIBUTES) {
    std::cerr << "Failed to get file attributes for " << linkName << std::endl;
    return false;
  }

  if (!(attributes & FILE_ATTRIBUTE_REPARSE_POINT)) {
    return false;
  }
  return true;

}
#endif

// Installs the package located at the temporary directory (TEMP_DIR/current_package)
#ifndef TARGET_WINDOWS
std::pair<bool, std::string> ToolsInstall::installPackageFile (const std::filesystem::path packageFile, const std::vector<std::string> args)
#else
std::pair<bool, std::wstring> ToolsInstall::installPackageFile (const std::filesystem::path packageFile, const std::vector<std::string> args)
#endif
{

  // Extract the package to a temporary directory
  const std::filesystem::path tmpPackageDirectory = TEMP_DIR / "tempcontent";
  if (std::filesystem::exists(tmpPackageDirectory)) {
    std::filesystem::remove_all(tmpPackageDirectory);
  }
  std::filesystem::create_directories(tmpPackageDirectory);

  if (!extractLocalFile(packageFile, tmpPackageDirectory)) {
#ifndef TARGET_WINDOWS
    return std::pair<bool, std::string> (false, "Failed to extract package.");
#else
    return std::pair<bool, std::wstring> (false, L"Failed to extract package.");
#endif
  }

  // Ensure the existence of a soundcache directory
  std::filesystem::create_directories(tmpPackageDirectory / "maps");
  std::filesystem::create_directories(tmpPackageDirectory / "maps" / "soundcache");

  // Start Portal 2
  if (!startPortal2(args)) {
#ifndef TARGET_WINDOWS
    return std::pair<bool, std::string> (false, "Failed to start Portal 2. Is Steam running?");
#else
    return std::pair<bool, std::wstring> (false, L"Failed to start Portal 2. Is Steam running?");
#endif
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

  std::filesystem::path gamePath = std::filesystem::path(gameProcessPath).parent_path();
  std::cout << "Found Portal 2 at " << gamePath << std::endl;

  std::filesystem::path tempcontentPath = gamePath / "portal2_tempcontent";

  // Handle an existing tempcontent directory
  if (std::filesystem::exists(tempcontentPath)) {
    if (isDirectoryLink(tempcontentPath)) {
      // If it's a symlink or junction, just remove it
      unlinkDirectory(tempcontentPath);
    } else {
      // If it's an actual file/directory, move it away to be safe
      std::filesystem::path tempcontentBackupPath = gamePath.parent_path() / ".spplice_tempcontent_backup";
      std::filesystem::create_directories(tempcontentBackupPath);
      // Check for the next available backup slot
      for (int i = 1; i <= 64; i ++) {
        std::filesystem::path newPath = tempcontentBackupPath / ("portal2_tempcontent_" + i);
        if (std::filesystem::exists(newPath)) continue;
        std::filesystem::rename(tempcontentPath, newPath);
      }
    }
  }

  // Link the extracted package files to the destination tempcontent directory
  if (!linkDirectory(tmpPackageDirectory, tempcontentPath)) {
#ifndef TARGET_WINDOWS
    return std::pair<bool, std::string> (false, "Failed to link package files to portal2_tempcontent.");
#else
    return std::pair<bool, std::wstring> (false, L"Failed to link package files to portal2_tempcontent.");
#endif
  }

  // Link the soundcache from base Portal 2 to skip waiting for it to generate
  linkFile(gamePath / "portal2" / "maps" / "soundcache" / "_master.cache", tempcontentPath / "maps" / "soundcache" / "_master.cache");

  // Run the JavaScript entrypoint (if one exists) on a detached thread
  const std::filesystem::path jsEntryPoint = tmpPackageDirectory / "main.js";
  if (std::filesystem::exists(jsEntryPoint)) {
    std::thread jsThread([jsEntryPoint]() {
      ToolsJS::runFile(jsEntryPoint);
    });
    jsThread.detach();
  }

  // Report install success to the UI
#ifndef TARGET_WINDOWS
  return std::pair<bool, std::string> (true, gamePath.string());
#else
  return std::pair<bool, std::wstring> (true, gamePath.wstring());
#endif

}

#ifndef TARGET_WINDOWS
void ToolsInstall::Uninstall (const std::string &gamePath)
#else
void ToolsInstall::Uninstall (const std::wstring &gamePath)
#endif
{

  // Remove the tempcontent directory link
  const std::filesystem::path tempcontentPath = std::filesystem::path(gamePath) / "portal2_tempcontent";
  unlinkDirectory(tempcontentPath);

  // Remove the actual package directory containing all package files
  const std::filesystem::path tmpPackageDirectory = TEMP_DIR / "tempcontent";
  std::filesystem::remove_all(tmpPackageDirectory);

}

// Despite the name of this file, these are generic tools for downloading files/strings
// These only use CURL on Linux, as Windows has its own approach

#include <iostream>
#include <fstream>
#include <string>
#include <filesystem>

#include "../globals.h" // Project globals

// Definitions for this source file
#include "curl.h"

#ifndef TARGET_WINDOWS
#include "curl/curl.h"

// CURL write callback function for appending to a string
size_t ToolsCURL::curlStringWriteCallback (void *contents, size_t size, size_t nmemb, void *userp) {
  ((std::string*)userp)->append((char*)contents, size *nmemb);
  return size *nmemb;
}

// CURL write callback function for writing to a file
size_t ToolsCURL::curlFileWriteCallback (void *contents, size_t size, size_t nmemb, void *userp) {
  std::ofstream *ofs = static_cast<std::ofstream *>(userp);
  size_t totalSize = size * nmemb;
  ofs->write(static_cast<const char *>(contents), totalSize);
  return totalSize;
}

// Downloads a file from the specified URL to the specified path, returns true if successful
bool ToolsCURL::downloadFile (const std::string &url, const std::filesystem::path outputPath) {

  std::ofstream ofs(outputPath, std::ios::binary);

  if (!ofs.is_open()) {
    std::cerr << "Failed to open file for writing: " << outputPath << std::endl;
    return false;
  }

  // Initialize CURL
  CURL *curl = curl_easy_init();

  if (!curl) {
    std::cerr << "Failed to initialize CURL" << std::endl;
    return false;
  }

  // Set request parameters
  curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
  curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, ToolsCURL::curlFileWriteCallback);
  curl_easy_setopt(curl, CURLOPT_WRITEDATA, &ofs);

  CURLcode response = curl_easy_perform(curl);

  // Clean up CURL
  curl_easy_cleanup(curl);

  if (response != CURLE_OK) {
    std::cerr << "Failed to download file from \"" << url << "\": " << curl_easy_strerror(response) << std::endl;
    return false;
  }

  return true;

}

// Downloads and returns a string from the given URL
std::string ToolsCURL::downloadString (const std::string &url) {

  // Initialize CURL
  CURL *curl = curl_easy_init();

  if (!curl) {
    std::cerr << "Failed to initialize CURL" << std::endl;
    return "";
  }

  std::string readBuffer;

  // Set request parameters
  curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
  curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, ToolsCURL::curlStringWriteCallback);
  curl_easy_setopt(curl, CURLOPT_WRITEDATA, &readBuffer);

  // Perform the request
  CURLcode response = curl_easy_perform(curl);

  // Check for errors
  if (response != CURLE_OK) {
    std::cerr << "Failed to download string from \"" << url << "\": " << curl_easy_strerror(response) << std::endl;
    return "";
  }

  // Clean up CURL
  curl_easy_cleanup(curl);

  return readBuffer;

}

#endif // ifndef TARGET_WINDOWS

#ifdef TARGET_WINDOWS
#include <windows.h>
#include <wininet.h>

bool ToolsCURL::downloadFile (const std::string &url, const std::filesystem::path outputPath) {

  // Convert std::string to std::wstring
  std::wstring wideUrl(url.begin(), url.end());
  std::wstring wideOutputPath = outputPath.wstring();

  HINTERNET hInternet = InternetOpenW(L"File Downloader", INTERNET_OPEN_TYPE_DIRECT, NULL, NULL, 0);
  if (!hInternet) {
    std::cerr << "InternetOpenW failed: " << GetLastError() << std::endl;
    return false;
  }

  HINTERNET hConnect = InternetOpenUrlW(hInternet, wideUrl.c_str(), NULL, 0, INTERNET_FLAG_RELOAD, 0);
  if (!hConnect) {
    std::cerr << "InternetOpenUrlW failed: " << GetLastError() << std::endl;
    InternetCloseHandle(hInternet);
    return false;
  }

  HANDLE hFile = CreateFileW(wideOutputPath.c_str(), GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
  if (hFile == INVALID_HANDLE_VALUE) {
    std::cerr << "CreateFileW failed: " << GetLastError() << std::endl;
    InternetCloseHandle(hConnect);
    InternetCloseHandle(hInternet);
    return false;
  }

  char buffer[4096];
  DWORD bytesRead;
  BOOL result;

  while ((result = InternetReadFile(hConnect, buffer, sizeof(buffer), &bytesRead)) && bytesRead > 0) {
    DWORD bytesWritten;
    WriteFile(hFile, buffer, bytesRead, &bytesWritten, NULL);
    if (bytesWritten != bytesRead) {
      std::cerr << "WriteFile failed: " << GetLastError() << std::endl;
      CloseHandle(hFile);
      InternetCloseHandle(hConnect);
      InternetCloseHandle(hInternet);
      return false;
    }
  }

  if (!result) {
    std::cerr << "InternetReadFile failed: " << GetLastError() << std::endl;
  }

  CloseHandle(hFile);
  InternetCloseHandle(hConnect);
  InternetCloseHandle(hInternet);
  return true;

}

std::string ToolsCURL::downloadString (const std::string &url) {

  // Convert std::string to std::wstring
  std::wstring wideUrl(url.begin(), url.end());

  // Initialize WinINet
  HINTERNET hInternet = InternetOpenW(L"String Downloader", INTERNET_OPEN_TYPE_DIRECT, NULL, NULL, 0);
  if (!hInternet) {
    std::cerr << "InternetOpenW failed: " << GetLastError() << std::endl;
    return "";
  }

  // Open the URL
  HINTERNET hConnect = InternetOpenUrlW(hInternet, wideUrl.c_str(), NULL, 0, INTERNET_FLAG_RELOAD, 0);
  if (!hConnect) {
    std::cerr << "InternetOpenUrlW failed: " << GetLastError() << std::endl;
    InternetCloseHandle(hInternet);
    return "";
  }

  // Read data from the URL
  std::string result;
  char buffer[4096];
  DWORD bytesRead;
  BOOL readResult;

  while ((readResult = InternetReadFile(hConnect, buffer, sizeof(buffer), &bytesRead)) && bytesRead > 0) {
    result.append(buffer, bytesRead);
  }

  if (!readResult) {
    std::cerr << "InternetReadFile failed: " << GetLastError() << std::endl;
  }

  // Clean up
  InternetCloseHandle(hConnect);
  InternetCloseHandle(hInternet);

  return result;

}

#endif // ifdef TARGET_WINDOWS

#include <iostream>
#include <fstream>
#include <string>
#include <filesystem>

// Don't get these mixed up! This is the actual CURL header
#include "curl/curl.h"
// ... and this is the header with definitions for this source file
#include "curl.h"

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

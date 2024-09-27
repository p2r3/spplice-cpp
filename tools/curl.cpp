#include <iostream>
#include <fstream>
#include <string>
#include <filesystem>

#include "../globals.h" // Project globals

// Definitions for this source file
#include "curl.h"

#ifndef TARGET_WINDOWS
  #include "../deps/linux/include/curl/curl.h"
#else
  #include "../deps/win32/include/curl/curl.h"
#endif

// Initializes CURL globally
void ToolsCURL::init () {
  curl_global_init(CURL_GLOBAL_DEFAULT);
}
// Cleans up CURL globally
void ToolsCURL::cleanup () {
  curl_global_cleanup();
}

// CURL write callback function for appending to a string
size_t curlStringWriteCallback (void *contents, size_t size, size_t nmemb, void *userp) {
  ((std::string*)userp)->append((char*)contents, size *nmemb);
  return size *nmemb;
}

// CURL write callback function for writing to a file
size_t curlFileWriteCallback (void *contents, size_t size, size_t nmemb, void *userp) {
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
  curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, curlFileWriteCallback);
  curl_easy_setopt(curl, CURLOPT_WRITEDATA, &ofs);
  curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);

#ifdef TARGET_WINDOWS
  // TODO: Build CURL with Schannel on Windows
  // This should *NOT* be here in the final release
  curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0L);
#endif

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
  curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, curlStringWriteCallback);
  curl_easy_setopt(curl, CURLOPT_WRITEDATA, &readBuffer);
  curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);

#ifdef TARGET_WINDOWS
  // TODO: Build CURL with Schannel on Windows
  // This should *NOT* be here in the final release
  curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0L);
#endif

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

// Creates a WebSocket connection, returns the respective CURL handle
CURL* ToolsCURL::wsConnect (const std::string &url) {

  // Initialize CURL
  CURL *curl = curl_easy_init();

  if (!curl) {
    std::cerr << "Failed to initialize CURL" << std::endl;
    return nullptr;
  }

  curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
  curl_easy_setopt(curl, CURLOPT_CONNECT_ONLY, 2L);

#ifdef TARGET_WINDOWS
  // TODO: Build CURL with Schannel on Windows
  // This should *NOT* be here in the final release
  curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0L);
#endif

  // Perform the request
  CURLcode response = curl_easy_perform(curl);

  // Check for errors
  if (response != CURLE_OK) {
    std::cerr << "Failed to connect to \"" << url << "\": " << curl_easy_strerror(response) << std::endl;
    return nullptr;
  }

  return curl;

}

// Disconnects a WebSocket connection and cleans up the CURL handle
void ToolsCURL::wsDisconnect (CURL *curl) {

  // Send a WebSocket close event
  size_t sent;
  curl_ws_send(curl, "", 0, &sent, 0, CURLWS_CLOSE);

  // Clean up CURL
  curl_easy_cleanup(curl);

}

bool ToolsCURL::wsSend (CURL *curl, const std::string &message) {

  // Send the given message as text
  size_t sent;
  CURLcode response = curl_ws_send(curl, message.c_str(), message.length(), &sent, 0, CURLWS_TEXT);

  // Log errors, return false if response not OK
  if (response != CURLE_OK) {
    std::cerr << "Failed to send message to WebSocket: " << curl_easy_strerror(response) << std::endl;
    return false;
  }

  return true;

}

std::string ToolsCURL::wsReceive (CURL *curl, size_t size) {

  // Set up a buffer for the incoming message
  size_t rlen;
  char buffer[size];

  // Receive the message
  const struct curl_ws_frame *meta;
  CURLcode response = curl_ws_recv(curl, buffer, sizeof(buffer), &rlen, &meta);

  // If the socket is not ready, assume there's just no data to be read
  if (response == CURLE_AGAIN) return "";

  // Log errors, return empty string if response not OK
  if (response != CURLE_OK) {
    std::cerr << "Failed to receive message from WebSocket: " << curl_easy_strerror(response) << std::endl;
    return "";
  }

  // Return a string representation of the buffer
  return std::string(buffer, rlen);

}

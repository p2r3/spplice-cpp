// CURL write callback function for appending to a string
size_t curlStringWriteCallback (void *contents, size_t size, size_t nmemb, void *userp) {
  ((std::string*)userp)->append((char*)contents, size *nmemb);
  return size *nmemb;
}

// CURL write callback function for writing to a file
size_t curlFileWriteCallback(void *contents, size_t size, size_t nmemb, void *userp) {
  std::ofstream *ofs = static_cast<std::ofstream *>(userp);
  size_t totalSize = size * nmemb;
  ofs->write(static_cast<const char *>(contents), totalSize);
  return totalSize;
}

// Downloads a file to the application's temp directory from the specified URL, returns path to file
std::string downloadTempFile (CURL *curl, const std::string &url, const std::string &filename) {

  std::filesystem::path outputPath = TEMP_DIR/filename;

  std::ofstream ofs(outputPath, std::ios::binary);

  if (!ofs.is_open()) {
    std::cerr << "Failed to open file for writing: " << outputPath << std::endl;
    return nullptr;
  }

  curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
  curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, curlFileWriteCallback);
  curl_easy_setopt(curl, CURLOPT_WRITEDATA, &ofs);

  CURLcode response = curl_easy_perform(curl);

  if (response != CURLE_OK) {
    std::cerr << "Failed to download file from \"" << url << "\": " << curl_easy_strerror(response) << std::endl;
    return nullptr;
  }

  return outputPath;

}

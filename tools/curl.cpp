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

// Downloads a file from the specified URL to the specified path, returns true if successful
bool downloadFile (const std::string &url, const std::filesystem::path outputPath) {

  std::cout << "creating outstream" << std::endl;
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

  std::cout << "setting url " << url.c_str() << std::endl;
  curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
  std::cout << "setting writefunc" << std::endl;
  curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, curlFileWriteCallback);
  std::cout << "setting outstream" << std::endl;
  curl_easy_setopt(curl, CURLOPT_WRITEDATA, &ofs);

  std::cout << "performing download" << std::endl;
  CURLcode response = curl_easy_perform(curl);
  std::cout << "download finished" << std::endl;

  // Clean up CURL
  curl_easy_cleanup(curl);

  if (response != CURLE_OK) {
    std::cerr << "Failed to download file from \"" << url << "\": " << curl_easy_strerror(response) << std::endl;
    return false;
  }

  return true;

}

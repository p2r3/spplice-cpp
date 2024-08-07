// Fetches and parses repository JSON from the given URL
std::vector<PackageData> fetchRepository (CURL *curl, const char *url) {

  std::string readBuffer;

  // Set the URL for the request
  curl_easy_setopt(curl, CURLOPT_URL, url);

  // Set the callback function to handle the data
  curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, curlStringWriteCallback);
  curl_easy_setopt(curl, CURLOPT_WRITEDATA, &readBuffer);

  // Perform the request
  CURLcode response = curl_easy_perform(curl);

  // Check for errors
  if (response != CURLE_OK) {
    std::cerr << "Failed to fetch repository \"" << url << "\": " << curl_easy_strerror(response) << std::endl;
    // Soft-fail by returning an empty vector
    return std::vector<PackageData>();
  }

  // Parse the repository index
  rapidjson::Document doc;
  doc.Parse(readBuffer.c_str());

  // Count the packages in the repository
  rapidjson::Value &packages = doc["packages"];
  const int packageCount = packages.Size();

  // Create a vector, since a dynamic array is a bit more of a pain in the ass
  std::vector<PackageData> repository;
  for (int i = 0; i < packageCount; i ++) {
    repository.push_back(PackageData(packages[i]));
  }

  return repository;

}

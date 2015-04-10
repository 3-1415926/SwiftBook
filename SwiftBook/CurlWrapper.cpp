#include "CurlWrapper.h"

#include <sstream>

using std::string;

string CurlWrapper::Get(const string& url) {
  std::ostringstream stream;
  Save(url, stream);
  return stream.str();
}

void CurlWrapper::Save(const string& url, std::ostream& stream) {
  char error_message[CURL_ERROR_SIZE];
  curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
  curl_easy_setopt(curl, CURLOPT_ERRORBUFFER, &error_message);
  curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION,
      &CurlWrapper::WriteCallback<std::ostream>);
  curl_easy_setopt(curl, CURLOPT_WRITEDATA, &stream);
  CURLcode error_code = curl_easy_perform(curl);
  if (error_code != 0) {
    throw "Error " + std::to_string(error_code) + ": " + error_message;
  }
}

template <typename Stream>
size_t CurlWrapper::WriteCallback(
    void *buffer, size_t size, size_t nmemb, void *userp) {
  Stream* stream = static_cast<Stream*>(userp);
  stream->write(static_cast<const char*>(buffer), size * nmemb);
  return size * nmemb;
}

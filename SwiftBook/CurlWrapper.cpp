#include "CurlWrapper.h"

#include <sstream>

using std::string;
using std::stringstream;

string CurlWrapper::Get(const string& url) {
  stringstream ss;
  char error_message[CURL_ERROR_SIZE];
  curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
  curl_easy_setopt(curl, CURLOPT_ERRORBUFFER, &error_message);
  curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, &CurlWrapper::WriteCallback);
  curl_easy_setopt(curl, CURLOPT_WRITEDATA, &ss);
  CURLcode error_code = curl_easy_perform(curl);
  if (error_code != 0) {
    throw "Error " + std::to_string(error_code) + ": " + error_message;
  }
  return ss.str();
}

size_t CurlWrapper::WriteCallback(
    void *buffer, size_t size, size_t nmemb, void *userp) {
  stringstream* ss = static_cast<stringstream*>(userp);
  ss->write(static_cast<const char*>(buffer), size * nmemb);
  return size * nmemb;
}

#include "CurlWrapper.h"

#include <sstream>

using std::string;
using std::stringstream;

string CurlWrapper::Get(const string& url) {
  stringstream ss;
  curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
  curl_easy_setopt(curl, CURLOPT_ERRORBUFFER, &error_message);
  curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, &CurlWrapper::WriteCallback);
  curl_easy_setopt(curl, CURLOPT_WRITEDATA, &ss);
  error_code = curl_easy_perform(curl);
  return ss.str();
}

size_t CurlWrapper::WriteCallback(
    void *buffer, size_t size, size_t nmemb, void *userp) {
  stringstream* ss = static_cast<stringstream*>(userp);
  ss->write(static_cast<const char*>(buffer), size * nmemb);
  return size * nmemb;
}

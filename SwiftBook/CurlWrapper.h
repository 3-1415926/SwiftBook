#ifndef __CurlWrapper__
#define __CurlWrapper__

#include <curl.h>
#include <easy.h>
#include <string>
#include <vector>

class CurlWrapper {
 public:
  CurlWrapper() {
    curl = curl_easy_init();
  }

  virtual ~CurlWrapper() {
    curl_easy_cleanup(curl);
  }

  std::string Get(const std::string& url);

  const char* LastErrorMessage() { return error_message; }
  const int LastErrorCode() { return error_code; }

 private:
  static size_t WriteCallback(
      void *buffer, size_t size, size_t nmemb, void *userp);

  CURL* curl;
  CURLcode error_code;
  char error_message[CURL_ERROR_SIZE];
};

#endif /* defined(__CurlWrapper__) */

#ifndef __CurlWrapper__
#define __CurlWrapper__

#include <curl.h>
#include <easy.h>
#include <ostream>
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
  void Save(const std::string& url, std::ostream& stream);

 private:
  template <typename Stream>
  static size_t WriteCallback(
      void *buffer, size_t size, size_t nmemb, void *userp);

  CURL* curl;
};

#endif /* defined(__CurlWrapper__) */

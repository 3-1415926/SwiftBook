#ifndef __SwiftBook__Book__
#define __SwiftBook__Book__

#include <string>
#include <ostream>
#include <sstream>
#include <vector>

#include "Chapter.h"
#include "CurlWrapper.h"

class Book {
 public:
  bool ReadFrom(const std::string& root_url);
  bool WriteTo(const std::string& out_dir);

 private:
  bool ReadToc();
  bool ReadEachChapter();

  std::string SaveResource(const std::string& url, const std::string& out_dir);

  CurlWrapper curl_wrapper;
  std::string root_url;
  std::vector<Chapter> chapters;
  std::string preamble;
  const std::string epilogue { "</body></html>" };
  std::string resource_root;
};

#endif /* defined(__SwiftBook__Book__) */

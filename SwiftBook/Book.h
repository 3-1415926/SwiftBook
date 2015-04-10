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
  void ReadFrom(const std::string& root_url);
  void WriteTo(const std::string& out_dir);

 private:
  void ReadToc();
  void ReadEachChapter();

  std::string SaveResource(const std::string& url);
  std::string LocalizeUrls(const std::string& text);
  std::string JoinPath(const std::vector<std::string>& args);

  CurlWrapper curl_wrapper;
  std::string root_url;
  std::string out_dir;
  std::vector<Chapter> chapters;
  std::string preamble;
  const std::string epilogue { "</body></html>" };
};

#endif /* defined(__SwiftBook__Book__) */

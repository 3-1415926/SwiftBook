#ifndef SwiftBook_RegexHelper_h
#define SwiftBook_RegexHelper_h

#include <sstream>
#include <regex>

bool StartsWith(const std::string& text, const std::string& with) {
  return std::equal(with.begin(), with.end(), text.begin());
}

std::string Search(const std::string& text, const std::regex& regex,
    int group = 0) {
  std::smatch match;
  return std::regex_search(text, match, regex) ? match.str(group) : "";
}

std::string Search(const std::string& text, const std::string& pattern,
    int group = 0) {
  return Search(text, std::regex(pattern), group);
}

std::string Replace(const std::string& text, const std::regex& regex,
    std::function<std::string(std::smatch)> replacement) {
  std::stringstream output;
  size_t last_pos = 0;
  for (std::sregex_iterator it(text.begin(), text.end(), regex), end;
       it != end; it++) {
    output.write(text.data() + last_pos, it->position() - last_pos);
    output << replacement(*it);
    last_pos = it->position() + it->length();
  }
  output.write(text.data() + last_pos, text.size() - last_pos);
  return output.str();
}

std::string Replace(const std::string& text, const std::string& pattern,
    std::function<std::string(std::smatch)> replacement) {
  return Replace(text, std::regex(pattern), replacement);
}

#endif

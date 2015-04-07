#ifndef SwiftBook_Chapter_h
#define SwiftBook_Chapter_h

struct Topic {
  Topic(std::string anchor, std::string name) : anchor(anchor), name(name) { }
  std::string anchor;
  std::string name;
};

struct Chapter : Topic {
  Chapter(bool has_text,
          std::string rel_path, std::string anchor, std::string name)
      : Topic(anchor, name), has_text(has_text), rel_path(rel_path) { }

  bool has_text;
  std::string rel_path;
  std::string text;
  std::vector<Topic> sub_topics;
};

#endif

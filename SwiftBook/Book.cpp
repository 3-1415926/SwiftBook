#include "Book.h"

#include <fstream>
#include <iostream>
#include <regex>

#include "RegexHelper.h"

using std::cout;
using std::endl;
using std::string;

std::regex abs_regex("^https?://", std::regex::icase);
std::regex url_attr_regex("(src|href)=\"([^\"]*)\"", std::regex::icase);
std::regex no_res_regex("\\.(html|zip)$", std::regex::icase);

void Book::ReadFrom(const string& root_url) {
  this->root_url = root_url;
  this->chapters.clear();
  ReadToc();
  ReadEachChapter();
}

void Book::ReadToc() {
  cout << "Getting index..." << endl;
  const string index = curl_wrapper.Get(root_url);
  const string nav_str = Search(index, "<nav(?:.|\n)*?</nav>");
  if (nav_str.size() == 0) {
    throw "Could not match the index";
  }

  std::regex nav_link("<li data-id=\"()([^\"]*)\".*?>(.*)"
                     "|<a href=\"([^\"#]*)#?([^\"]*)\".*?>([^<]*)</a>");
  for (std::sregex_iterator it(nav_str.begin(), nav_str.end(), nav_link), end;
       it != end; it++) {
    bool has_text = !(*it)[3].matched;
    int from = has_text ? 3 : 0;
    chapters.emplace_back(has_text,
        it->str(1 + from).size() == 0 ? "index.html" : it->str(1 + from),
        it->str(2 + from),
        it->str(3 + from));
    cout << chapters.rbegin()->name << endl;
  }
  if (chapters.size() == 0) {
    throw "Could not match any TOC link";
  }

  preamble = Search(index, "^(?:.|\n)*<body[^>]*>");
  if (preamble.size() == 0) {
    throw "Could not match preamble";
  }
}

void Book::ReadEachChapter() {
  cout << "Reading all chapters:" << endl;
  std::regex content_regex("<article class=\"chapter\">((?:.|\n)*?)"
      "<section id=\"next_previous\"");
  std::regex jump_regex("<menuitem id=\"jump_to\"(?:.|\n)*?</menuitem>");
  std::regex jump_item_regex("<a href=\"#([^\"]*)\".*?>([^<]*)</a>");
  for (auto& chapter : chapters) {
    if (!chapter.has_text) {
      continue;
    }

    cout << "Reading " << chapter.rel_path << endl;
    const string text = curl_wrapper.Get(
        JoinPath({ root_url, chapter.rel_path }));

    chapter.text = Search(text, content_regex, 1);
    if (chapter.text.size() == 0) {
      throw "Could not extract content from: " + chapter.rel_path;
    }

    const string jump_str = Search(text, jump_regex);
    if (jump_str.size() == 0) {
      cout << "  (no jump_to menu)" << endl;
      continue;
    }

    for (std::sregex_iterator it(jump_str.begin(), jump_str.end(),
                                 jump_item_regex), end;
         it != end; it++) {
      chapter.sub_topics.emplace_back(it->str(1), it->str(2));
    }
    if (chapter.sub_topics.size() == 0) {
      throw "Could not find any sub-topics in: " + chapter.rel_path;
    }
  }
}

string Book::SaveResource(const string& url) {
  cout << "Saving: " << url << endl;

  const string name = Search(url, "[^/]+$");
  if (name.size() == 0) {
    throw "Could not match url file name: " + url;
  }

  const string abs_url = std::regex_search(url, abs_regex) ? url
      : JoinPath({ root_url, url });
  const string out_path = JoinPath({ out_dir, name });
  std::ofstream output(out_path, std::fstream::trunc);
  if (!output.good()) {
    throw "Resource output stream is not good for: " + out_path;
  }
  curl_wrapper.Save(abs_url, output);
  output.close();
  return name;
}

string Book::LocalizeUrls(const string& text) {
  return Replace(text, url_attr_regex,
      [this](std::smatch match) {
        string url = match.str(2);
        if (match.str(1) == "src") {
          url = SaveResource(url);
        } else /* if (match.str(1) == "href") */ {
          if (!std::regex_search(url, abs_regex) && url.size() > 0) {
            size_t hash_pos = url.find_first_of('#');
            if (hash_pos != string::npos) {
              if (url.substr(0, hash_pos).find('/')) {
                url = JoinPath({ root_url, url });
              } else {
                url = url.substr(hash_pos);
              }
            } else if (std::regex_search(url, no_res_regex)) {
              url = JoinPath({ root_url, url });
            } else {
              url = SaveResource(url);
            }
          }
        }
        return match.str(1) + "=\"" + url + "\"";
      });
}

string Book::JoinPath(const std::vector<string>& args) {
  std::stringstream ss;
  bool is_first = *args.begin()->begin() != '/';
  for (const auto& it : args) {
    if (it.size() == 0) {
      continue;
    }
    int prefix_len = *it.begin() == '/';
    int suffix_len = *it.rbegin() == '/';
    if (!is_first) {
      ss << '/';
    }
    ss << it.substr(prefix_len, it.size() - prefix_len - suffix_len);
    is_first = false;
  }
  if (*args.rbegin()->rbegin() == '/') {
    ss << '/';
  }
  return ss.str();
}

void Book::WriteTo(const std::string& out_dir) {
  this->out_dir = out_dir;
  const string out_path = JoinPath({ out_dir, "swift-doc.html" });
  std::ofstream output(out_path, std::fstream::trunc);
  if (!output.good()) {
    throw "Output stream is not good: " + out_path;
  }

  cout << "Writing preamble..." << endl;
  output << LocalizeUrls(preamble) << endl;

  cout << "Writing TOC..." << endl;
  output << "<h2 class='chapter-name'>Table of Contents</h2>" << endl;
  output << "<section class='section'>" << endl;
  output << "<ul style='font-size: 150%'>" << endl;
  for (const auto& chapter : chapters) {
    output << "\t<li class='nav-chapter' style='" << (chapter.has_text ?
        "text-indent: 10pt" : "font-weight: bold") << "'>" << endl;
    output << "\t\t<a href=\"#" << chapter.anchor << "\">"
           << chapter.name << "</a>" << endl;
    output << "\t</li>" << endl;
    if (chapter.sub_topics.size() > 0) {
      output << "\t<ul style='text-indent: 20pt; font-style: italic'>" << endl;
      for (const auto& sub_topic : chapter.sub_topics) {
        output << "\t\t<li class='nav-chapter'>"
               << "<a href=\"#" << sub_topic.anchor
               << "\">" << sub_topic.name << "</a></li>" << endl;
      }
      output << "\t</ul>" << endl;
    }
  }
  output << "</ul>" << endl;
  output << "</section>" << endl;
  output << "<hr/>" << endl;
  for (const auto& chapter : chapters) {
    cout << "Writing chapter: " << chapter.name << endl;
    if (chapter.has_text) {
      output << endl << "<!-- " << chapter.name << " -->" << endl << endl;
      output << LocalizeUrls(chapter.text) << endl;
    }
  }

  cout << "Writing epilogue..." << endl;
  output << epilogue << endl;

  output.close();
}



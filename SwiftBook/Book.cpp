#include "Book.h"

#include <fstream>
#include <iostream>
#include <regex>

#include "RegexHelper.h"

using std::cout;
using std::endl;
using std::string;

std::regex abs_regex("^https?://", std::regex::icase);

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
        it->str(1 + from), it->str(2 + from), it->str(3 + from));
    cout << chapters.rbegin()->name << endl;
  }
  if (chapters.size() == 0) {
    throw "Could not match any TOC link";
  }

  preamble = Search(index, "^(?:.|\n)*<body[^>]*>");
  if (preamble.size() == 0) {
    throw "Could not match preamble";
  }

  preamble = Replace(preamble, "<meta id=\"resources-uri\" "
      "name=\"resources-uri\" content=\"([^\"]*)\">",
      [this](std::smatch match) {
        this->resource_root = match.str(1);
        return "";
      });
  if (resource_root.size() == 0) {
    throw "Could not match resources-uri";
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
    const string text = curl_wrapper.Get(root_url + chapter.rel_path);

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

  string name = Search(url, "[^/]+$");
  if (name.size() == 0) {
    throw "Could not match url file name: " + url;
  }

  const string abs_url = std::regex_match(url, abs_regex)
      ? url : StartsWith(url, resource_root)
          ? root_url + "/" + url
          : root_url + "/" + resource_root + "/" + url;
  const string data = curl_wrapper.Get(url);
  const string out_path = out_dir + "/" + name;
  std::ofstream output(out_path, std::fstream::trunc);
  if (!output.good()) {
    throw "Resource output stream is not good for: " + out_path;
  }

  output.write(data.data(), data.size());
  output.close();
  return name;
}

std::regex url_attr_regex("(src|href)=\"([^\"]*)\"");
string Book::LocalizeUrls(const string& text) {
  return Replace(text, url_attr_regex,
      [this](std::smatch match) {
        string new_url;
        if (match.str(1) == "src") {
          new_url = this->SaveResource(match.str(2));
        } else /* if (match.str(1) == "href") */ {
          size_t hash_pos = match.str(2).find_first_of('#');
          if (std::regex_match(match.str(2), abs_regex)) {
            new_url = match.str(2);
          } else if (hash_pos != string::npos) {
            new_url = match.str(2).substr(hash_pos);
          }
          else {
            throw "Relative url without hash anchor: " + match.str(2);
          }
        }
        return match.str(1) + "=\"" + new_url + "\"";
      });
}

void Book::WriteTo(const std::string& out_dir) {
  this->out_dir = out_dir;
  const string out_path = out_dir + "/swift-doc.html";
  std::ofstream output(out_path, std::fstream::trunc);
  if (!output.good()) {
    throw "Output stream is not good: " + out_path;
  }

  cout << "Writing preamble..." << endl;
  output << LocalizeUrls(preamble) << endl;

  cout << "Writing TOC..." << endl;
  std::function<void(const string& tag, const Topic* topic)> output_toc_item =
      [&output](const string& tag, const Topic* topic) {
        output << "<" << tag << "><a href=\"#" << topic->anchor << "\">"
               << topic->name << "</a></" << tag << ">" << endl;
      };
  output << "<h2>Table of Contents</h2>" << endl;
  for (const auto& chapter : chapters) {
    output_toc_item(chapter.has_text ? "h4" : "h3", &chapter);
    for (const auto& sub_topic : chapter.sub_topics) {
      output_toc_item("h5", &sub_topic);
    }
  }
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



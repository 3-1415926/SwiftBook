#include "Book.h"

#include <fstream>
#include <iostream>
#include <regex>

#include "RegexHelper.h"

using std::cout;
using std::endl;
using std::string;

bool Book::ReadFrom(const string& root_url) {
  this->root_url = root_url;
  this->chapters.clear();
  if (!ReadToc()) { return false; }
  if (!ReadEachChapter()) { return false; }
  return true;
}

bool Book::ReadToc() {
  cout << "Getting index..." << endl;
  const string index = curl_wrapper.Get(root_url);
  if (curl_wrapper.LastErrorCode() != 0) {
    cout << "Error " << curl_wrapper.LastErrorCode()
         << " when reading index." << endl
         << "Message: " << curl_wrapper.LastErrorMessage() << endl;
    return false;
  }

  const string nav_str = Search(index, "<nav(?:.|\n)*?</nav>");
  if (nav_str.size() == 0) {
    cout << "Could not match the index." << endl;
    return false;
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
    cout << "Could not match any TOC link." << endl;
    return false;
  }

  preamble = Search(index, "^(?:.|\n)*<body[^>]*>");
  if (preamble.size() == 0) {
    cout << "Could not match preamble." << endl;
    return false;
  }

  resource_root = Search(index, "<meta id=\"resources-uri\" "
      "name=\"resources-uri\" content=\"([^\"]*)\">");

  return true;
}

bool Book::ReadEachChapter() {
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
    string text = curl_wrapper.Get(root_url + chapter.rel_path);
    if (curl_wrapper.LastErrorCode() != 0) {
      cout << "Error " << curl_wrapper.LastErrorCode()
           << " when reading chapter: " << chapter.rel_path << endl
           << "Message: " << curl_wrapper.LastErrorMessage() << endl;
      return false;
    }

    chapter.text = Search(text, content_regex, 1);
    if (chapter.text.size() == 0) {
      cout << "Could not extract content from: " << chapter.rel_path << endl;
      return false;
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
      cout << "Could not find any sub-topics in: " << chapter.rel_path << endl;
      return false;
    }
  }
  return true;
}

string Book::SaveResource(const string& url, const string& out_dir) {
  string name = Search(url, "[^/]+$");
  if (name.size() == 0) {
    cout << "Could not match url file name: " << url << endl;
    return "";
  }

  string abs_url = StartsWith(url, "http://") || StartsWith(url, "https://")
      ? url : StartsWith(url, resource_root)
          ? root_url + "/" + url
          : root_url + "/" + resource_root + "/" + url;
  string data = curl_wrapper.Get(url);
  if (curl_wrapper.LastErrorCode() != 0) {
    cout << "Error " << curl_wrapper.LastErrorCode()
         << " when reading resource: " << url << endl
         << "Message: " << curl_wrapper.LastErrorMessage() << endl;
    return "";
  }

  std::ofstream output(out_dir + "/" + name, std::fstream::trunc);
  if (!output.good()) {
    cout << "Resource output stream is not good!" << endl;
    return "";
  }

  output.write(data.data(), data.size());
  output.close();
  return name;
}


bool Book::WriteTo(const std::string& out_dir) {
  cout << "Writing output..." << endl;
  std::ofstream output(out_dir + "/swift-doc.html", std::fstream::trunc);
  if (!output.good()) {
    cout << "Output stream is not good!" << endl;
    return false;
  }

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
    if (chapter.has_text) {
      output << endl << "<!-- " << chapter.name << " -->" << endl << endl;
      output << chapter.text << endl;
    }
  }

  output.close();
  return true;
}



#include <iostream>
#include <string>

#include "Book.h"

using std::cout;
using std::endl;
using std::string;

int main(int argc, const char * argv[]) {
  Book book;
  if (!book.ReadFrom("https://developer.apple.com/library/ios/documentation/"
      "Swift/Conceptual/Swift_Programming_Language/")) {
    cout << "Failed to read the book." << endl;
    return 1;
  }
  if (!book.WriteTo(".")) {
    cout << "Failed to write the book." << endl;
    return 1;
  }
  cout << "Done!" << endl;
}

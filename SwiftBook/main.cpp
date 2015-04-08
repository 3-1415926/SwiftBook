#include <iostream>
#include <string>

#include "Book.h"

using std::cout;
using std::endl;
using std::string;

int main(int argc, const char * argv[]) {
  try {
    Book book;
    book.ReadFrom("https://developer.apple.com/library/ios/documentation/"
      "Swift/Conceptual/Swift_Programming_Language/");
    book.WriteTo(".");
  } catch (string ex) {
    cout << ex << endl;
    return 1;
  }
  cout << "Done!" << endl;
  return 0;
}

// string::find
#include <iostream>
#include <string>
using namespace std;

int main ()
{
  string str ("123_There are two needles in this haystack with needles.");
  string str2 ("_");
  size_t found = str.find(str2);
  cout << found << endl; 

  return 0;
}

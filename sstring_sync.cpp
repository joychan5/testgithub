// read a file into memory
#include <iostream>
using namespace std;

int main () {
  char first, second;

  cout << "Please, enter a word: ";
  first=cin.get();
  cin.sync();

  cout << "Please, enter another word: ";
  second=cin.get();

  cout << "The first word began by " << first << endl;
  cout << "The second word began by " << second << endl;

  return 0;
}

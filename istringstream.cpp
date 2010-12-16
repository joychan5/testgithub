// using istringstream constructors.
#include <iostream>
#include <sstream>
#include <string>
using namespace std;

int main () {

  //int n,val;
  string val;
  string stringvalues;

  stringvalues = "125 320 512 750 333";
  istringstream iss (stringvalues,istringstream::in);

  iss >> val;
  cerr << val << endl;
  return 0;
}

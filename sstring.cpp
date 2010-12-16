// modify basefield
#include <iostream>
#include <sstream>
using namespace std;

int main () {
  stringstream ss(stringstream::out);
  double a,b,c;
  a = 3.1415926534;
  b = 2006.0;
  c = 1.0e-10;
  ss.precision(5);
  ss       <<         a << '\t' << b << '\t' << c << endl;
  ss <<   fixed    << a << '\t' << b << '\t' << c << endl;
  ss << scientific << a << '\t' << b << '\t' << c << endl;
  ss.sync();
  cout << ss.str() << endl;
  return 0;
}

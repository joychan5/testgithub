#include <iostream>
#include <limits>
#include <cstdlib>
#include <cstring>
#include <cstdio>
typedef std::numeric_limits< double > dbl;

using namespace std;
int main(){
	double a = 1e-05/1000/1000/1000/1000000;
	double b = a * 1000*1000*1e3*1e5*1e6;
	double c = 1234567891234567;
	cout << a << endl;
	cout << b << endl;
	cout.precision(dbl::digits10);
	cout << "precision" << dbl::digits10 << endl;
	cout << c << endl;
        char s[64] = {0};
        snprintf( s, sizeof(s), "%.12f", 0.019336);
	double x= 1.2010100;
        cout << x << endl;
        fprintf(stderr,"%f\n", x);
        fprintf(stderr,"%g\n", x);
	return 0;

}

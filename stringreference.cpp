#include <iostream>
using namespace std;

void modify( string &a ){
	a = "modified";
	return ;
}
int main(){
	string a = "test string";
	cout << a << endl;
	modify(a);
	cout << a << endl;
	return 0;
}

#include <iostream>
#include <string>
#include <map>

using namespace::std;

class Test{
public:
	Test():id("default"){cerr << "Test() construct " << endl;}
	Test( const Test &orig)
	:id(orig.id)
	{
		cerr << "Test( const Test &orig) construct " << endl;
	}
	Test(string a):id(a){cerr << "Test(string) construct " << endl;}
	Test & operator=(const Test &orig){
		if( this != &orig ){
			this->id = orig.id;
		}
		cerr << "operator=(const Test &orig) " << endl;
		return *this;
	}
	~Test(){cerr << "deconstruct Test:" << id << endl;}
	string getid(){return id;}

private:
	string id;
};

map<int,Test> imap;

int main(){
    //pair<int,Test>(0,new Test());
    //imap.insert( make_pair(0, Test("test")) );
    //imap.insert( pair<int,Test>(0, Test("test")) );
    imap[0] = Test("test");
    cerr << "imap[0].getid() = " << imap[0].getid() << endl;
    cerr << "destruct imap[0]" << endl;
    //imap.erase(0); 
    cerr << "after erase(0)" << endl;
    return 0;
}

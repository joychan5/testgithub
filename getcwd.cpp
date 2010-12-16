#include <iostream>

std::string my_pwd()
{
 char buf[1024];
 int count;
 count=readlink("/proc/self/exe",buf,1024);
 if(count<0 || count>=1024)
  return "";
 buf[count]='\0';
 return buf;
}
int main(){

std::string cwd = my_pwd();
std::cout<<__FILE__<<std::endl;
return 0;
}


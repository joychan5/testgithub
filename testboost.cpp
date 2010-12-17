#include <boost/regex.hpp>
#include <iostream>
#include <string>
using namespace std;
int main()
{
    std::string line;
    cout << "strlen(\"天下无敌\") = " << strlen("天下无敌") << endl;
    char *dest = "天下无敌";
    cout << "strstr(\"天下无敌\",\"无敌\") = " << strstr( dest ,"无敌") - dest << endl;
    boost::regex pat( "(^.*(天下无敌|中国人民|共产党|老子).*)" );
    //boost::regex pat( "^Sub天ject: (Re: |Aw: )*(.*)" );

    while (std::cin)
    {
        std::getline(std::cin, line);
        boost::smatch matches;
        if (boost::regex_match(line, matches, pat))
            //std::cout << matches[0] << std::endl;
            //std::cout << matches[1] << std::endl;
            std::cout << matches[2] << std::endl;
            //std::cout << matches[3] << std::endl;
    }
}


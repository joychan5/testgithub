/*
 *
 * Copyright (c) 2003
 * John Maddock
 *
 * Use, modification and distribution are subject to the 
 * Boost Software License, Version 1.0. (See accompanying file 
 * LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
 *
 */

 /*
  *   LOCATION:    see http://www.boost.org for most recent version.
  *   FILE         regex_iterator_example_2.cpp
  *   VERSION      see <boost/version.hpp>
  *   DESCRIPTION: regex_iterator example 2: searches a cpp file for class definitions,
  *                using global data.
  */

#include <string>
#include <map>
#include <fstream>
#include <iostream>
#include <boost/regex.hpp>

using namespace std;

// purpose:
// takes the contents of a file in the form of a string
// and searches for all the C++ class definitions, storing
// their locations in a map of strings/int's

typedef std::map<std::string, std::string::difference_type, std::less<std::string> > map_type;

/*const char* re = 
   // possibly leading whitespace:   
   "^[[:space:]]*" 
   // possible template declaration:
   "(template[[:space:]]*<[^;:{]+>[[:space:]]*)?"
   // class or struct:
   "(class|struct)[[:space:]]*" 
   // leading declspec macros etc:
   "("
      "\\<\\w+\\>"
      "("
         "[[:blank:]]*\\([^)]*\\)"
      ")?"
      "[[:space:]]*"
   ")*" 
   // the class name
   "(\\<\\w*\\>)[[:space:]]*" 
   // template specialisation parameters
   "(<[^;:{]+>)?[[:space:]]*"
   // terminate in { or :
   "(\\{|:[^;\\{()]*\\{)";
*/
//const char *re = "^.*(天下无敌|共产党).*";
const char *re = "^.*(0-898|0536jiancai|08-98|089-8|1413711.qidian.com|218vip|21ao1.info|365m8.com|400-657-1988|5161w.cn|blackstone|blackstone-sh|cdc988|chinaredwood|退谠|退出中共的党|胡紧掏|胡jin套|零八宪章|驗証碼|黑石上海|flickr.com/photos/chinamove/sets/72157623360211983/detail/|FUCKGFW|fun.18eka.com|ggii.net|hejingyuan|hhg5.info|hsdezx|http://sinaxl.qrqr.net/|http://www.31do.cn|http://xl09k.cn/|http://xt-qq699.cn|liangyanjing|lunsha.com|mizhuangbaihe|mycool.net|pay.cnhubei.com|qrqr.net|shengqiang.net|shop36820513|sinaxl.qrqr.net|timediy.com|u.115.com/file/t078c40285|wangGongQuan.net|www.bin001.com|www.byi520.com|www.liangyanjing.cn|www.manyso.com|zuilink|zuilink.cn|zuilink.com|zuilink.net|傲慢的留法学生OoYaYaoO|共铲党|坦克压学生|天灭中共|打倒共产党|杜春娇|江责民|江氏控制权力|一亩站|世界最大的私募基金|京V02009|亮眼睛（拼音）|何竟|何竟源).*";

boost::regex expression;
map_type class_index;

bool regex_callback(const boost::match_results<std::string::const_iterator>& what)
{
   // what[0] contains the whole string
   // what[5] contains the class name.
   // what[6] contains the template specialisation if any.
   // add class name and position to map:
   //class_index[what[0].str()] = what.position(0);
   //class_index[what[2].str()] = what.position(2);
   //class_index[what[3].str()] = what.position(3);
   class_index[what[1].str()] = what.position(1);
   return true;
}

void load_file(std::string& s, std::istream& is)
{
   s.erase();
   if(is.bad()) return;
   s.reserve(is.rdbuf()->in_avail());
   char c;
   while(is.get(c))
   {
      if(s.capacity() == s.size())
         s.reserve(s.capacity() * 3);
      s.append(1, c);
   }
}

int main(int argc, const char** argv)
{
expression = re;
   std::string text;
   for(int i = 1; i < argc; ++i)
   {
      cout << "Processing file " << argv[i] << endl;
      std::ifstream fs(argv[i]);
      load_file(text, fs);
      fs.close();
      // construct our iterators:
      boost::sregex_iterator m1(text.begin(), text.end(), expression);
      boost::sregex_iterator m2;
      std::for_each(m1, m2, &regex_callback);
      // copy results:
      cout << class_index.size() << " matches found" << endl;
      map_type::iterator c, d;
      c = class_index.begin();
      d = class_index.end();
      while(c != d)
      {
         cout << "class \"" << (*c).first << "\" found at index: " << (*c).second << endl;
         ++c;
      }
      class_index.erase(class_index.begin(), class_index.end());
   }
   return 0;
}




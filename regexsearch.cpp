#include <string>
#include <map>
#include <fstream>
#include <iostream>
#include <boost/regex.hpp>
#include <string> 
using namespace std;
// purpose: 
// takes the contents of a file in the form of a string 
// and searches for all the C++ class definitions, storing 
// their locations in a map of strings/int's 
typedef std::map<std::string, int, std::less<std::string> > map_type; 
//boost::regex expression("^.*何竟.*");
boost::regex expression("^.*(0-898|0536jiancai|08-98|089-8|1413711.qidian.com|218vip|21ao1.info|365m8.com|400-657-1988|5161w.cn|blackstone|blackstone-sh|cdc988|chinaredwood|退谠|退出中共的党|胡紧掏|胡jin套|零八宪章|驗証碼|黑石上海|flickr.com/photos/chinamove/sets/72157623360211983/detail/|FUCKGFW|fun.18eka.com|ggii.net|hejingyuan|hhg5.info|hsdezx|http://sinaxl.qrqr.net/|http://www.31do.cn|http://xl09k.cn/|http://xt-qq699.cn|liangyanjing|lunsha.com|mizhuangbaihe|mycool.net|pay.cnhubei.com|qrqr.net|shengqiang.net|shop36820513|sinaxl.qrqr.net|timediy.com|u.115.com/file/t078c40285|wangGongQuan.net|www.bin001.com|www.byi520.com|www.liangyanjing.cn|www.manyso.com|zuilink|zuilink.cn|zuilink.com|zuilink.net|傲慢的留法学生OoYaYaoO|共铲党|坦克压学生|天灭中共|打倒共产党|杜春娇|江责民|江氏控制权力|一亩站|世界最大的私募基金|京V02009|亮眼睛（拼音）|何竟|何竟源|克林顿国务卿关于互联|网自由的讲|话希拉里|∙克林顿（H|illaryRodh|amClinton）|国务卿华盛顿哥伦比|亚特区新闻博物馆（N|ewseum）|2009年1|月21日|（星期四）非常感谢，|艾伯托（|Albert|o）。不仅要|感谢你的赞|誉和介绍，而且|要感谢你和你的同|事们在这个|重要机构中发|挥的领导作用。很高|兴来到新闻博物馆。这|个博物馆是一座纪|念碑，见证了我|们最珍视的一些自由。|我十分感谢能|有此机会|谈谈如何运用这些自由|应对二十一世纪的各项|挑战。虽然|我并不能|看到你们所有的人—|—因为在这样|的场合灯光|照射我的眼|睛，而你们都|在背光处—|—但我知道在座的有很|多朋友和老同|事。我要感谢自由|论坛（Fr|eedomF|orum）的首席|执行官查尔|斯∙奥弗比（Cha|rlesOverby|）光临新闻博物|馆，以及我在|参议院时的老同|事理查德∙卢格（|RichardL|ugar）|和乔∙利伯曼(J|oeLie|berman)两位|参议员，他们两位|都为《表达法》（|VoiceAct）的|通过作出了努力。|这项立法表明，|美国国会|和美国人民不分党派|，不分政府部|门，坚定地支持|互联网自由。我听说在|场的还有参议员萨|姆∙布朗巴克（Sa|mBro|wnback）|、参议员|特德∙考夫曼（Te|dKaufman）|、众议员洛雷塔|∙桑切斯（Lo|rett|aSanc|hez）、许|多大使、临时代|办和外交使团的|其他代表、以及从中国|、哥伦比亚|、伊朗、|黎巴嫩和摩|尔多瓦等国前来参加我|们关于互联网|自由的“国际访问者领|袖计划”（In|ternationa|lVisitorL|eader|shipPro|gram）的人士。|我还要提到最近被任命|为广播理事会（Br|oadcas|tingBoardo|fGoven|ors）|理事的阿|斯彭研究所（Aspe|nInst|itute）所长沃|尔特∙艾萨克森（W|alterIsa|acson|）。毫无疑问，他在阿|斯彭研究所从事的支|持互联网自由的工|作中发挥了重要|作用。这是|关于一个非常重|要的议题的一个重|要讲话。但在|开始谈这个议题前，|我想简要介绍一下|海地的情况。过去|八天来，海地人民|和世界人民|携手应对一场巨|大的灾难。我们|这个半球曾历|经磨难，但我们目前在|太子港面临的|困境鲜有先例|。通讯网络在我们抗|击这场灾难的过程|中发挥了极|其重要的作|用。不用说，当地的|通讯网络遭受|了重创，在|很多地方被彻底摧毁。|地震发生|后仅几个小时，我们就|与民营部门的伙|伴发起“海地|”（HAITI）短|信捐款活动，|使美国的|移动电话使|用者能通过发短|信向救灾工作捐款|。这项活动充分展示了|美国人民的慷慨。迄今|，该活动已为|海地的抗震救灾筹|集了2500多|万美元。信息网络在救|灾现场也发挥了极其|重要的作|用。星期六，我|在太子港会|见普雷瓦|尔（Preval）|总统时，他的重点|目标之一是要努力恢复|通讯。幸存的通|讯设施不足以帮助当地|政府官员|相互联络，|非政府组织|以及我们的文|职部门和|军队的领导人|的运作能力|都受到严重影响。|高科技公司设立了|互动地图|，帮助确定救灾需要和|目标资源。就在星期|一，一名年仅|七岁的小女孩和|两名妇女通过|发短信呼救被|一个美国搜救|队从坍塌的超市的|残砖碎瓦下救了出来。|这些事例只是一个普遍|现象的缩影。信息|网络的扩展正在为|我们的星球建立一个新|的神经系|统。在海地或湖南发生|什么情况|时，我们其余的人都能|从当事者那里实|时得知。我们还可以实|时作出反应。|灾后迫切希望|提供帮助|的美国人和被困在|超市瓦砾下的小|姑娘以一年以|前乃至一代人以前|还想象不到的方式被|联系在一起。今天|，同样的原则|适用于几乎整个|人类。我们今天坐|在这里，你|们中间任何|人——或|更有可能的是我们孩|子中的任何人—|—都可以拿出|很多人每|天随身携带|的通讯工具|，将这次讨论的内容发|送给全世界数十亿人|。在很多方面，信息从|未像今天这么自由。与|过去任何时候相比，今|天都有更多的方式把更|多的想法|传播给更多的人。即|使在集权国家|，信息网络也在帮助人|们发现新的|事实，向政府更多地|问责。例|如，奥巴马总统11|月访华期间|与当地大学生的|直接对话包含了|网上提问|，突显了|互联网的重要性。在|回答一个网上提问|时，他强调人民有权|自由获取|信息。他说，信息流通|越自由，社会就越强|健。他谈到获取信息|的权力如何有助|于公民向|自己的政府问责，激|发新的想法，鼓励|创造性和创业|精神。我今天来这里|发表讲话正是出|于美国对这|一经过实践检验的真|理的信念。由|于人们的|相互联系空前密切，|我们也必|须认识到这些新|技术并非无条件地造|福人类。这些|工具也正被|用于阻碍人类进步和剥|夺政治权|利。正如钢可被|用于建造医院|也可用于制造机枪|。核能可为城市提|供动力也可摧|毁城市。现代|信息网络及|其支持的|技术既可被用|于行善也|可被用于作恶。有助|于组织自|由运动的网络也能|使“基地”组织得以煽|动仇恨，挑起针对无辜|者的暴力。|具有开放政|府信息和促进透明化|潜力的技术也可被|政府劫持，用|于镇压异见，剥|夺公民权|利。过去一年来|，我们看到对|信息自由流|通的威胁激增。中国、|突尼斯和|乌兹别克斯坦加|强了对互联|网的审查。在越南，使|用广受欢迎的社交|网站的权利突然消|失。上个星|期五在埃及|，30名博|客作者和维权人士|被拘留。这批博|客作者中的一位|是巴塞姆∙萨米尔|（BassemSam|ir）。他有幸获|释，今天也在这|里，同我们在一起。因|此，一方面|，这些技术的|推广明显地正在改变我|们的世界，另一|方面，尚无法预知|这样的改变将对世|界人民的人|权和幸福|产生何种|影响。这些|新技术本身不会|在争取自由与进|步的斗争中选|择立场。但是，|美国要做到立场|鲜明。我们支持一个允|许全人类平等享有|知识和思想的互|联网。而且我们|认识到，在世|界上建立何种信息基|础设施将取决于我|们和其他人为之确|定的性质。虽|然这是一|个全新的|挑战，但|我们确保思想自由交流|的责任可追溯至和众|国诞生之初。《宪|法》第一修正案的内容|字字镌刻在这座大楼前|那块50|吨重的田纳西大理石上|。世世代代的美|国人都为捍卫刻在|那块石头上的价|值观付出了努力。富|兰克林•罗斯福（|Franklin|Roosevelt）|在1941|年发表“|四项自由”演|讲时发扬了这些思|想。当时，美国人|面临着一系|列的危机，此|外还有信心|危机。但是，对一|个人人都享有言论表|达自由、信仰自由、没|有贫困、没|有恐惧的世界|的憧憬冲破了|他那个时代的|重重困难。多年|之后，我的楷模|之一艾琳娜•|罗斯福（E|lean|orRoos|evelt|）努力使这|些原则成为|《世界人权宣言》的|奠基原则。这些原则|成为继往开来每一代|人的北斗，引导|我们、鞭策我们|、促使我们在|险恶的环境中勇|于向前。在科学技|术飞跃发展的时|候，我们必须反思这|个传统。我们|需要确保|科学技术|的进步与我们的原则|同步。在接|受诺贝尔奖时，奥|巴马总统讲到需|要建设这样|一个世界，让和平建立|在每一个人固有的权利|和尊严之上。几|天后在乔治敦|大学关于人权的|演讲中，|我表示我们必须探|索途径，把人权变|成现实。|今天，我|们迫切需要在二十一世|纪的电子世界|中保护这些|自由。世界上有许|多其他的|网络，有|些帮助人员|或资源的流动，有|些辅助志同道合|的个人之间的交流|。但互联网|是增强所有其他网络的|能力和潜力的|一个网络，因此，|我们认为确保其使用者|享有某些基本自|由至关重要。|其中最重要|的是言论表|达自由。这种自由的定|义不再仅仅是|公民前往|市政厅前的广场批评他|们的政府，而不担心遭|受报复。博客、|电子邮件、社|交网络和手机短信开启|到我们所|能给予的每一|个机会的年轻人。|非常感谢|你们。（掌).*"); 

void IndexClasses(map_type& m, const std::string& file) 
{ 
   std::string::const_iterator start, end; 
   start = file.begin(); 
   end = file.end(); 
      boost::match_results<std::string::const_iterator> what; 
   boost::match_flag_type flags = boost::match_default; 
   //while(regex_search(start, end, what, expression, flags)) 
   //{ 
      // what[0] contains the whole string 
      // what[5] contains the class name. 
      // what[6] contains the template specialisation if any. 
      // add class name and position to map: 
regex_search(start, end, what, expression, flags);
      m[std::string(what[1].first, what[1].second)] = what[1].first - file.begin(); 
      // update search position: 
      start = what[0].second; 
      // update flags: 
      flags |= boost::match_prev_avail; 
      flags |= boost::match_not_bob; 
	cout << "test" << endl;
   //} 
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

   	std::string text;
	cout << "Processing file " << argv[1] << endl;
	std::ifstream fs(argv[1]);
	load_file(text, fs);
	fs.close();
	map_type match;
	IndexClasses(match,text);
	// copy results:
	cout << match.size() << " matches found" << endl;
	map_type::iterator c, d;
	c = match.begin();
	d = match.end();
	while(c != d)
	{
	 cout << "class \"" << (*c).first << "\" found at index: " << (*c).second << endl;
	 ++c;
	}
	match.erase(match.begin(), match.end());
   
   return 0;
}


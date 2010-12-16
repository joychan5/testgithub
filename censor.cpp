/*
 * File:   censor.cpp
 * Author: freedomhui
 * Usage:
 * Compiling: c++ censor.cpp sae_function.c /usr/local/lib/libboost_regex.a -o censor -g
 *
 * Created on March 2, 2010, 1:49 PM
 */

#include "sae_function.h"
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <signal.h>
#include <stdio.h>
#include <cstdlib>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <sys/socket.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <sys/wait.h>


#include <boost/regex.hpp>
#include <iostream>
#include <vector>
#include <string>
#include <map>
using namespace std;
#define WORD_LIST_FILE   "./SensitiveWords.txt"
#define LOG_FILE         "./censor.log"
#define ASSERT_FILE      "./assert.log"
#define REFRESH_INTERVAL 600

#define MSG_SIZE         100
#define CODE_LEN         4
#define RT_STR_LEN       11
#define MAX_CLIENTS      95
#define PORT             "1123"
#define BACKLOG          100
#define SAE_LINE_MAX     250
#define TIME_LEN         40
#define FIRST_INIT_FLAG  -101
#define PUNC_LEN         5
#define SHORTEST_MSG_LEN strlen("NNN:N|")

/* processCensor Result Code */
#define NO_SENSITIVE_WORD       302
#define HAS_SENSITIVE_WORD      303
#define PROCESS_ERROR           304
#define OTHERS                  305


#define PHP_ASSERT_NUM(n1,n2)  		do {				\
	long num1 = (n1);                                   \
        long num2 = (n2);                               \
        current_time( time_str, TIME_LEN );                             \
        if( num1 != num2 ){                                  \
              fprintf( log_fp, "%s - ERROR: Integer Assert Fail( %ld != %ld ). f(%s), l(%d)\n", time_str, num1, num2, __FILE__, __LINE__);    \
        }                                                     \
} while (0)

#define PHP_ASSERT_STR(s1,s2)  		do {				\
	const char * str1 = (s1);                                   \
        const char * str2 = (s2);                               \
        current_time( time_str, TIME_LEN );                             \
        if( str1 != str2 ){                                  \
              fprintf( assert_fp, "%s - ERROR: String Assert Fail( %s != %s ). f(%s), l(%d)\n", time_str, str1, str2, __FILE__, __LINE__);      \
        }                                                     \
} while (0)

#define ADD_LOG(loginfo)  		do {				\
        const char * info = (loginfo);                                  \
        current_time( time_str, TIME_LEN );                             \
        fprintf( log_fp, "%s - %s - f(%s), l(%d)\n", time_str, info, __FILE__, __LINE__); \
} while (0)

#ifdef DEBUG
#define DEBUG_LOG(loginfo)  		do {				\
        const char * info = (loginfo);                                  \
        current_time( time_str, TIME_LEN );                             \
        fprintf( log_fp, "%s - %s - f(%s), l(%d)\n", time_str, info, __FILE__, __LINE__); \
} while (0)
#else
#define DEBUG_LOG(lognone)          do{}while(0)
#endif

FILE *log_fp;
FILE *assert_fp;
vector<string> wordlist;
string re;	//regular expression for sensitive words censor
boost::regex expression;
struct stat buffer;
time_t lastmtime;
char time_str[TIME_LEN] = {0};
string tmp_log_msg;
char tmp_short_msg[MSG_SIZE];
char tmp_num_str[RT_STR_LEN] = {0};

const char punc[][PUNC_LEN] = {"，","。","；","？","！","：","、","，",",",".",";","?","!",":"};

typedef std::map<std::string, std::string::difference_type, std::less<std::string> > map_type;


static int setNonblocking(int fd)
{
    int flags;

    /* If they have O_NONBLOCK, use the Posix way to do it */
#if defined(O_NONBLOCK)
    /* Fixme: O_NONBLOCK is defined but broken on SunOS 4.1.x and AIX 3.2.5. */
    if (-1 == (flags = fcntl(fd, F_GETFL, 0)))
        flags = 0;
    return fcntl(fd, F_SETFL, flags | O_NONBLOCK);
#else
    /* Otherwise, use the old way of doing it */
    flags = 1;
    return ioctl(fd, FIONBIO, &flags);
#endif
}

static int setRcvTimeout(int fd, int sec ){
    struct timeval tv;
    tv.tv_sec = sec;  /* 30 Secs Timeout */
    setsockopt(fd, SOL_SOCKET, SO_RCVTIMEO,(struct timeval *)&tv,sizeof(struct timeval));
}

static void initreg( int signum ){
	FILE * fp;
	char word[SAE_LINE_MAX];
	time_t tmp_lastmtime;
        size_t wdlen;
	if( stat( WORD_LIST_FILE , &buffer) != -1 ){
		tmp_lastmtime = buffer.st_mtime;
		if( tmp_lastmtime <= lastmtime && signum != FIRST_INIT_FLAG ){
                    DEBUG_LOG("<=");
                    alarm(REFRESH_INTERVAL);
			return;
		}else{
                    DEBUG_LOG("else");
                    lastmtime = tmp_lastmtime;
                    wordlist.clear();
                }
	}else{
                ADD_LOG("ERROR: Fail to stat WORD_LIST_FILE.");
		exit(1);
	}

	fp = fopen( WORD_LIST_FILE, "r" );
	if( fp == NULL ){
            ADD_LOG("ERROR: Fail to open sensitive wordlist file");
            exit(1);
	}
	while( fgets(word, SAE_LINE_MAX, fp) != NULL ){
                wdlen = strlen(word);
                if( word[wdlen-1] = '\n' ){
                    word[wdlen-1] = '\0';
                }else{
                    continue;
                }
		wordlist.push_back( string(word) );
	}
	int i = 0;
	re.clear();
	re = "^.*(";
	for( i =0; i < wordlist.size(); ++i ){
		re += wordlist[i]+"|";
	}
	re.erase( re.size() - 1 );
	re.append(").*");
        expression = re;  /* update sensitive words' regular expression */
	fclose(fp);
        ADD_LOG("NOTICE: Update WordList Regex Successfull.");
	//do regex building
        cerr << "RegEx:" << re << endl;
	alarm(REFRESH_INTERVAL);
}


int processCensor( const std::string text, long *pos, long *len, int* result ){
        map_type match_index;
        if( text.size() <= 0 ){
            (*result) = PROCESS_ERROR;
            ADD_LOG("ERROR:text.size() <= 0");
            return -1;
        }
        std::string::const_iterator start, end;
        start = text.begin();
        end = text.end();
        boost::match_results<std::string::const_iterator> what;
        boost::match_flag_type flags = boost::match_default;
        while(regex_search(start, end, what, expression, flags))
        {
            match_index[std::string(what[1].first, what[1].second)] = what[1].first - text.begin();
          // update search position:
          start = what[0].second;
          // update flags:
          flags |= boost::match_prev_avail;
          flags |= boost::match_not_bob;
        }

	map_type::iterator rt = match_index.begin();
	if( match_index.size() <= 0 ){
            DEBUG_LOG("in processCensor 1");
		*pos = -1;
		*len = 0;
                (*result) = NO_SENSITIVE_WORD;
                match_index.erase(match_index.begin(), match_index.end());
		return 0;
	}else{
            DEBUG_LOG("in processCensor 3");
		*pos = (*rt).second;
		*len = (*rt).first.size();
                sprintf(tmp_short_msg,"NOTICE: Success to censor, pos=%ld, len=%ld",*pos,*len);
                DEBUG_LOG(tmp_short_msg);
                if( (*pos) >= 0 && (*len) > 0 ){
                    (*result) = HAS_SENSITIVE_WORD;
                }else{
                    (*result) = PROCESS_ERROR;
                }
                match_index.erase(match_index.begin(), match_index.end());
                DEBUG_LOG("in processCensor 2");
		return 0;
	}
}


void exitClient(int fd, fd_set *readfds, char fd_array[], int *num_clients){
    int i;
    
    close(fd);
    FD_CLR(fd, readfds); //clear the leaving client from the set
    for (i = 0; i < (*num_clients) - 1; i++)
        if (fd_array[i] == fd)
            break;          
    for (; i < (*num_clients) - 1; i++)
        (fd_array[i]) = (fd_array[i + 1]);
    (*num_clients)--;
}

int main(){
    	log_fp = fopen( LOG_FILE, "a+" );
        setbuf(log_fp,NULL);
        assert_fp = fopen( ASSERT_FILE, "a+");
        setbuf(assert_fp,NULL);
        if( log_fp == NULL || assert_fp == NULL ){
		fprintf( stderr, "Can't open log file\n");
		exit(1);
	}
	//daemonize("SAE_Censor");

        ADD_LOG( "NOTICE: Log Begin");//TEST
	if( stat( WORD_LIST_FILE , &buffer) != -1 ){
		lastmtime = buffer.st_mtime;
	}else{
		ADD_LOG( "ERROR: Fail to stat WORD_LIST_FILE.");
		exit(1);
	}
	initreg(FIRST_INIT_FLAG); /*Init regex variable for the first time*/
        expression = re;          /*Update expression for the first time*/
	alarm(REFRESH_INTERVAL);

	if( signal( SIGALRM, initreg ) == SIG_ERR ){
		ADD_LOG("ERROR:Fail to signal SIGALRM.");
	}

	int num_clients = 0;
	int server_sockfd, client_sockfd;/* listen on server_sockfd, new connection on client_fd */
	int fd;
        long result;/*the bytes read from socket*/
        size_t pos1, pos2; /* pos1,pos2 is used for extrat integer numbers from string */
        int pos_diff;
        long word_pos = 0; /* sensitive words' position */
        long word_len = 0; /* sensitive words' length */
	char fd_array[MAX_CLIENTS]; /* used for store sockets' fd */
	fd_set readfds, testfds;
	char msg[MSG_SIZE + 1];
        string message;

        string text; /* the text which needs to be sensored */
        char *text_;
	struct addrinfo hints, *servinfo, *p;
	struct sockaddr_storage their_addr; // connector's address information
	socklen_t sin_size;
	//struct sigaction sa;
	int reuse=1;
	int rv;
        char rtcode_str[CODE_LEN];  /*the connection status code in string*/
        long rtcode_num;
        string rtlen_str;
        long rtlen;
        long received_len;

        int censor_resultCode = 0;

	memset(&hints, 0, sizeof hints);
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_flags = AI_PASSIVE; // use my IP

	if ((rv = getaddrinfo(NULL, PORT, &hints, &servinfo)) != 0) {
                string gai_error = gai_strerror(rv);
                string errinfo = "ERROR: getaddrinfo: " + gai_error;
		ADD_LOG( errinfo.c_str() );
		return 1;
	}

	// loop through all the results and bind to the first we can
	for(p = servinfo; p != NULL; p = p->ai_next) {
		if ((server_sockfd = socket(p->ai_family, p->ai_socktype,
				p->ai_protocol)) == -1) {
			ADD_LOG("NOTICE:server: can't create socket");
			continue;
		}

		if (setsockopt(server_sockfd, SOL_SOCKET, SO_REUSEADDR, &reuse,
				sizeof(int)) == -1) {
			ADD_LOG("ERROR: can't setsockopt");
			exit(1);
		}

		if (bind(server_sockfd, p->ai_addr, p->ai_addrlen) == -1) {
			close(server_sockfd);
			ADD_LOG("server: bind");
			continue;
		}

		break;
	}

	if (p == NULL)  {
		ADD_LOG("ERROR: server failed to bind");
		return 2;
	}

	freeaddrinfo(servinfo); // all done with this structure

	if (listen(server_sockfd, BACKLOG) == -1) {
		ADD_LOG("listen");
		exit(1);
	}

/*
	sa.sa_handler = sigchld_handler; // reap all dead processes
	sigemptyset(&sa.sa_mask);
	sa.sa_flags = SA_RESTART;
	if (sigaction(SIGCHLD, &sa, NULL) == -1) {
		perror("sigaction");
		exit(1);
	}
*/
        FD_ZERO(&readfds);
        FD_SET(server_sockfd, &readfds);

	ADD_LOG("NOTICE: server is waiting for connections...");
	/*  Now wait for clients and requests */

	while(1) {  // main accept() loop
		testfds = readfds;
		select(FD_SETSIZE, &testfds, NULL, NULL, NULL);

		for (fd = 1; fd < FD_SETSIZE; fd++) {
			if (FD_ISSET(fd, &testfds)) {
				if (fd == server_sockfd) { /* Accept a new connection request */
					sin_size = sizeof their_addr;
					client_sockfd = accept(server_sockfd, (struct sockaddr *)&their_addr, &sin_size);
					if (num_clients < MAX_CLIENTS) {
                                                setNonblocking(client_sockfd);
                    				FD_SET(client_sockfd, &readfds);
                    				fd_array[num_clients++] = client_sockfd;
						sprintf( msg, "200: Hello, I'm waiting for you.\n", client_sockfd );
						send( client_sockfd, msg, strlen(msg), 0 );
					}else {
						sprintf( msg, "300: Sorry, too many clients. Try again later.\n" );
						write(client_sockfd, msg, strlen(msg));
						close(client_sockfd);
					}
				}else if(fd){
					//read data from open socket
                                        int k;
					result = read(fd, msg, MSG_SIZE);
					if( result > 0 ) {
						msg[result] = '\0';
					}else{
						ADD_LOG("ERROR: the bytes of read, result <= 0");
                                                exit(1);
					}

					strncpy( rtcode_str, msg, CODE_LEN-1 );
					rtcode_str[CODE_LEN-1] = '\0';
					rtcode_num = atoi(rtcode_str);
					//switch( rtcode_num ){
                                           if(rtcode_num == 201 ){ // case(201):
                                                message.clear();
                                                message.append(msg);
                                                
                                                pos1 = message.find(':')+1;
                                                pos2 = message.find('|');
                                                pos_diff = pos2 - pos1;
                                                if( pos1 == string::npos || pos2 == string::npos || pos_diff <=0 ){
                                                    //TODO,ERROR
                                                }
                                                rtlen_str = message.substr( pos1, pos_diff );                                                
                                                rtlen = atoi(rtlen_str.c_str());
                                                long tmp_len = rtlen + CODE_LEN + pos_diff + 1;
                                                //int count = 0;
                                                for(;;){
                                                    setRcvTimeout( fd , 3 );
                                                    if( result + MSG_SIZE >= tmp_len ){
                                                        k = read( fd, msg, tmp_len - result );
                                                        if( k > 0){
                                                            message.append(msg,k);
                                                            result += k;
                                                        }
                                                    }else{
                                                        k = read(fd, msg, MSG_SIZE);
                                                        if( k > 0){
                                                            message.append(msg,k);
                                                            result += k;
                                                        }
                                                    }       
                                                    memset(msg,0,MSG_SIZE + 1);
                                                    if( result >= tmp_len ){
                                                        break;
                                                    }
//                                                    count ++;
//                                                    if( count > (rtlen/MSG_SIZE)+1 ){
//                                                        ADD_LOG("out from here");
//                                                        break;
//                                                    }
                                                }
//                                                while( (k = read(fd, msg, MSG_SIZE)) != -1 ){
//                                                    message.append(msg,k);
//                                                    memset(msg,0,MSG_SIZE + 1);
//                                                    result += k;
//                                                }
                                                sprintf(tmp_num_str,"%ld",result);
                                                tmp_log_msg.clear();
                                                tmp_log_msg.append("NOTICE:Received:");
                                                tmp_log_msg.append(message.substr(0,100));
                                                tmp_log_msg.append("Len:");
                                                tmp_log_msg.append(tmp_num_str);
                                                ADD_LOG(tmp_log_msg.c_str());//TEST
                                                //exit(1);//for debug
                                                tmp_log_msg.clear();


                                                received_len = result-CODE_LEN-pos_diff-1;
                                                DEBUG_LOG(rtlen_str.c_str());
                                                PHP_ASSERT_NUM( rtlen, received_len );
                                                /*
                                                if( rtlen <= 0 || message.size() <= SHORTEST_MSG_LEN ){
                                                    ADD_LOG("ERROR: rtlen <= 0 || strlen(msg) <= SHORTEST_MSG_LEN ");
                                                    sprintf( msg, "204:Message too short!\n" );
                                                    send( fd, msg, strlen(msg), 0 );
                                                    exitClient(fd,&readfds, fd_array,&num_clients);
                                                    continue;
                                                }

                                                received_len = result - CODE_LEN - pos_diff - 1;
                                                if( received_len < rtlen ){
                                                    sprintf(tmp_log_msg,"NOTICE: received_len(%ld) < rtlen(%ld), Read Again.",received_len,rtlen);
                                                    ADD_LOG(tmp_log_msg);
                                                    text_ = (char *)malloc( sizeof(char) * rtlen );
                                                    result = read(fd, text_, rtlen);//TODO
                                                    text.append( msg+CODE_LEN+pos_diff+1, received_len );
                                                    text.append( text_, result );
                                                    free(text_);
                                                    PHP_ASSERT_NUM( text.size(), rtlen );
                                                }else if( received_len == rtlen ){
                                                    
                                                    text.append( msg+CODE_LEN+pos_diff+1, received_len );
                                                    sprintf(tmp_log_msg,"I'm feeling lucky. text=%s",text.c_str());
                                                    ADD_LOG(tmp_log_msg);//TEST
                                                }else{
                                                    
                                                    sprintf(tmp_log_msg,"ERROR: received_len(%ld) > rtlen(%ld)",received_len,rtlen);
                                                    ADD_LOG(tmp_log_msg);
                                                    continue;
                                                }
                                                */
                                                
                                                text = message.substr(CODE_LEN+pos_diff+1);
                                                message.clear();
                                                DEBUG_LOG(text.c_str());//TEST
                                                //exit(1);//for debug
                                                
                                                if( !processCensor( text, &word_pos, &word_len, &censor_resultCode ) ){
                                                    if( censor_resultCode == HAS_SENSITIVE_WORD ){
                                                        DEBUG_LOG("DEBUG:HAS_SENSITIVE_WORD");//for debug
                                                        sprintf( msg, "202:%ld|%ld\n", word_pos, word_len );
                                                        const int punc_size = sizeof(punc) / PUNC_LEN;
                                                        int i;
                                                        size_t left, right;
                                                        string result_str;
                                                        result_str.append(msg);
                                                        for( i = 0; i < punc_size; ++i ){
                                                            left = text.rfind( punc[i], word_pos );
                                                            if ( left != string::npos ){
                                                                break;
                                                            }
                                                        }
                                                        if ( left == string::npos ){
                                                            left = 0;
                                                        }else{
                                                            left += strlen(punc[i]);
                                                        }

                                                        for( i = 0; i < punc_size; ++i ){
                                                            right = text.find( punc[i], word_pos+word_len );
                                                            if ( right != string::npos ){
                                                                break;
                                                            }
                                                        }
                                                        if ( right == string::npos ){
                                                            right = text.size();
                                                        }
                                                        result_str.append( text.substr( left, right-left ) );
                                                        send( fd, result_str.c_str(), result_str.size(), 0 );
                                                        DEBUG_LOG(result_str.c_str());//for debug
                                                    }else{
                                                        DEBUG_LOG("DEBUG:up 203");//for debug
                                                        sprintf( msg, "203:none\n" );
                                                        send( fd, msg, strlen(msg), 0 );
                                                    }
                                                    exitClient(fd,&readfds, fd_array,&num_clients);
                                                }else{
                                                    DEBUG_LOG("DEBUG:up 204");//for debug
                                                    sprintf( msg, "204:Censor fail!\n" );
                                                    send( fd, msg, strlen(msg), 0 );
                                                    exitClient(fd,&readfds, fd_array,&num_clients);
                                                }
                                                text.clear();
                                                word_pos = word_len = censor_resultCode = 0;
                                                
                                           }else if(rtcode_num == 301 ){
                                                sprintf(tmp_short_msg,"NOTICE:Received:%s - Len:%ld ",msg,result);
                                                DEBUG_LOG(tmp_short_msg);
                                                exitClient(fd,&readfds, fd_array,&num_clients);
                                           }else{
                                               break;
                                           }
                                         
				}else{
					exitClient(fd,&readfds, fd_array,&num_clients);
				}
			}/* if */
		}/* for */
	}/* while(1) */
}/* main */



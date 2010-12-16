/*
 * Copyright (c) SINA zhiyong1@staff.sina.com.cn
 */

#ifndef CC_CYCLE_HPP__
#define CC_CYCLE_HPP__

#include <vector>
#include <map>
#include <list>
#include <string>
#include <sstream>
#include <cassert>
#include <errno.h>
#include <time.h>

#include <unistd.h>
#include <fcntl.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>

#include <curl/curl.h>
#include <event.h>

#include <ranker/ranker.hpp>

#include <cc_protocol.hpp>
#include <cc_log.hpp>

namespace Cc {

class Cycle;
class TcpConnection;

extern Cycle *currentCycle;

class TcpListening {
public:
	TcpListening(in_port_t port) : port_(port), fd_(-1) { }
	~TcpListening() { if (fd_ > 0) close(fd_);	}

	static void onAcceptWrap(int fd, short action, void *userp);
	void onAccept();

	bool listen(int backlog = 15);

public:
	Cycle *cycle;
	struct event ev;

private:
	in_port_t port_;
	int fd_;
};

class HttpBackEnd {
public:
	typedef enum {Ok, Not200} BackEndState;

public:
	TcpConnection *tc;
	CURL *easy;
	struct event ev;
	int action;
	bool evset;
	int fd;

	BackEndState state;
};

class TcpConnection {
public:

	typedef enum {Ok = 0, InternalError, ProtocolError, SendAgain} ReturnCode;

	void reset(int fd);

	void descRunnings();
	bool finished();

	static void doRecvWrap(int fd, short action, void *userp);
	void doRecv();

	static void doSendWrap(int fd, short action, void *userp);
	void doSend(ReturnCode rc = Ok);

public:
	Cycle *cycle;

private:
	struct event event_;
	bool evset_;

	size_t runnings_;
	std::map<CURL *, std::string> easyBackEnds_;

	TcpProtocol protocol_;
	std::string outputBuffer_;
	size_t bufferPos_;

	int fd_;
};

class Cycle {
public:

	Cycle();
	~Cycle();

	bool prepare(int maxfd);
	bool loop();

	void setTag(int i);
	int getTag() const;

	void updateTime();

	bool connectBackEnd(TcpConnection *tc, const UriMap &uriItems, std::map<CURL *, std::string> *easyBackEnds);
	void disconnectBackEnd(const std::map<CURL *, std::string> &easyBackEnds);
	HttpBackEnd *getBackEnd(CURL * const curl);
	HttpProtocol *getBackEndProtocol(CURL * const curl);

	static void timerCallbackWrap(int, short, void *userp);
	void timerCallback();

	static int multiCallbackWrap(CURL *e, int fd, int action, void *userp, void *socketp);
	int multiCallback(CURL *e, int fd, int action, HttpBackEnd *backEnd);

	static int multiTimerCallbackWrap(CURLM *multi, long ms, void *userp);
	int multiTimerCallback(long ms);

	void addBackEnd(CURL *e, int fd, int action, TcpConnection *tc);
	void setBackEnd(CURL *e, int fd, int action, TcpConnection *tc, HttpBackEnd *backEnd);
	void removeBackEnd(TcpConnection *tc, HttpBackEnd *backEnd);

	static void eventCallbackWrap(int fd, short event, void *userp);
	void eventCallback(int fd, short event, TcpConnection *tc);

	void checkRunCount(TcpConnection *tc = 0);

	static size_t backEndCallbackWrap(void *ptr, size_t size, size_t n, void *userp);
	size_t backEndCallback(const char *buffer, size_t size, CURL *easy);

	static size_t backEndHeaderCallbackWrap(void *ptr, size_t size, size_t n, void *userp);
	size_t backEndHeaderCallback(const char *buffer, size_t size, CURL *easy);

	bool checkEasyCurlPool(size_t n);

	void setTcpListening(TcpListening *tl);
	TcpListening *getTcpListening() ;

	void setRankerList(std::list<Ranker*> *rankerList);
	std::list<Ranker*> *getRankerList();

	TcpConnection *getConnection(int fd);
	HttpBackEnd *getBackEnd(TcpConnection *tc, CURL *e);
	
public:
	// struct event timerEvent;
	time_t currentTime;
	char currentTimeString[32];

private:
	TcpListening *listening_;
	std::list<Ranker *> *rankerList_;

	std::vector<TcpConnection> connectionPool_;

	std::vector<CURL *> easyCurlPool_;
	std::map<CURL *, HttpProtocol> backEndProtocolPool_;
	std::map<CURL *, HttpBackEnd> backEndPool_;

	CURLM *multi_;
	struct event timerEvent_;

	int  active_;
	size_t total_;

	int maxfd_;
	int tag_;
};

inline void TcpConnection::descRunnings()
{
	--runnings_;
}
inline bool TcpConnection::finished()
{
	return runnings_ == 0;
}

inline void Cycle::setTag(int i) { tag_ = i; }

inline int Cycle::getTag() const { return tag_; }

inline void Cycle::updateTime() 
{
	currentTime = time(NULL);
	strftime(currentTimeString, 31, "%Y-%m-%d %H:%M:%S", localtime(&currentTime));
}

inline void Cycle::setTcpListening(TcpListening *tl)
{
	listening_ = tl;
	listening_->cycle = this;
} 

inline TcpListening *Cycle::getTcpListening() { return listening_;	}

inline void Cycle::setRankerList(std::list<Ranker *> *rankerList)
{
	rankerList_ = rankerList;
}

inline std::list<Ranker *> *Cycle::getRankerList() 
{
	return rankerList_;
}

inline HttpBackEnd *Cycle::getBackEnd(CURL * const curl)
{
	return &backEndPool_[curl];
}

inline HttpProtocol *Cycle::getBackEndProtocol(CURL * const curl) 
{
	return &backEndProtocolPool_[curl];
}


} // namespace cc

#endif

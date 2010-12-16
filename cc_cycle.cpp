#include <cc_cycle.hpp>

namespace Cc {

Cycle *currentCycle = 0;

bool TcpListening::listen(int backlog) 
{
	struct sockaddr_in addr;
	memset(&addr, 0x00, sizeof(struct sockaddr_in));
	addr.sin_family = AF_INET;
	addr.sin_port = htons(port_);

	if ((fd_ = socket(addr.sin_family, SOCK_STREAM, 0)) == -1) {
		return false;
	}

	int reuseaddr = 1;
	if (setsockopt(fd_, SOL_SOCKET, SO_REUSEADDR, (const void *)&reuseaddr, sizeof(int)) == -1) {
		return false;
	}

	if (fcntl(fd_, F_SETFL, fcntl(fd_, F_GETFL) | O_NONBLOCK) == -1) {
		return false;	
	}

	if (bind(fd_, (struct sockaddr*)&addr, sizeof(struct sockaddr_in)) == -1) {
		return false;
	}

	if (::listen(fd_, backlog) == -1) {
		return false;	
	}

	event_set(&ev, fd_, EV_READ | EV_PERSIST, TcpListening::onAcceptWrap, this);

	return true;
}

void TcpListening::onAcceptWrap(int /*fd*/, short /* action */, void *userp) {
	TcpListening *me = static_cast<TcpListening *>(userp);
	me->onAccept();
}

void TcpListening::onAccept()
{
	int cfd = accept(fd_, 0, 0);
	if (cfd < 0) {
		log_error(errno, "accept failed");	
		return;
	}

	log_error(0, "accept fd %d", cfd);

	TcpConnection *tc = cycle->getConnection(cfd);
	if (!tc) {
		log_error(errno, "too many connections");
		close(cfd);
		return;
	}

	tc->reset(cfd);

	if (fcntl(cfd, F_SETFL, fcntl(fd_, F_GETFL) | O_NONBLOCK) == -1) {
		log_error(errno, "can't nonblock %d#%p", cfd, this);
		tc->doSend(TcpConnection::InternalError);
		return;
	}

	tc->doRecv();
}

void TcpConnection::doRecvWrap(int /* fd */, short /* action */, void *userp)
{
	TcpConnection *me = static_cast<TcpConnection *>(userp);
	me->doRecv();
}

void TcpConnection::doRecv()
{
	const size_t bufSize = 1024;
	char buf[bufSize];
	ssize_t n;
	while ((n = read(fd_, buf, bufSize)) > 0) {
		log_debug(0, "fd %d#%p recv %.*s", fd_, this, n, buf);
		protocol_.inputAppend(buf, n);	
	}

	if (n == -1) {
		if (errno != EAGAIN && errno != EINTR) {
			if (evset_) {
				evset_ = false;
				event_del(&event_);
			}
			log_debug(errno, "fd %d#%p ", fd_, this);
			doSend(InternalError);
			return;
		}
	}

	TcpProtocol::TcpProtocolStat protocolStat = protocol_.inputStat();
	if (protocolStat == TcpProtocol::Ok) {
		if (evset_) {
			evset_ = false;
			event_del(&event_);	
		}
		const UriMap &uriItems = protocol_.inputUriItems();
		runnings_ = uriItems.size();
		if (!cycle->connectBackEnd(this, uriItems, &easyBackEnds_)) {
			doSend(InternalError);
			return;
		}
	} else if (protocolStat == TcpProtocol::ProtocolRejected) {
		if (evset_) {
			evset_ = false;	
			event_del(&event_);
		}
		log_debug(0, "fd %d#%p protocol error", fd_, this);
		doSend(ProtocolError);
		return;
	} else {
		if (!evset_) {
			event_set(&event_, fd_, EV_READ | EV_PERSIST, TcpConnection::doRecvWrap, this);
			event_add(&event_, NULL);
			evset_ = true;
		}
	}

	return;
}

void TcpConnection::doSendWrap(int /* fd */, short /* action */, void *userp)
{
	TcpConnection *me = static_cast<TcpConnection *>(userp);
	me->doSend(SendAgain);
}
void TcpConnection::doSend(ReturnCode rc)
{
	static const char *returnCodeString[] = {"0:", "1:internal error\r\n\r\n", "2:protocol error\r\n\r\n"};

	if (rc != SendAgain) {
		if (rc != Ok) {
			outputBuffer_.assign(returnCodeString[rc]);
		} else {
			std::ostringstream os;
			os << returnCodeString[rc];
			for (std::map<CURL *, std::string>::iterator ite = easyBackEnds_.begin(), end = easyBackEnds_.end();
					ite != end; ++ite) {
				HttpBackEnd *httpBackEnd = cycle->getBackEnd(ite->first);
				if (httpBackEnd->state == HttpBackEnd::Ok) {
					HttpProtocol *httpProtocol = cycle->getBackEndProtocol(ite->first);
					if (httpProtocol->calcRank(cycle->getRankerList())) {
						int uriRank = httpProtocol->getRank();	
						os << "0@" << uriRank << '@' << ite->second;
					} else {
						log_error(0, "%s calcRank error", ite->second.c_str());
						os << "1@0@" << ite->second;
					}
				} else {
					log_error(0, "%s http request error", ite->second.c_str());
					os << "2@0@" << ite->second;	
				}
				os << "\r\n";
			}
			os << "\r\n";
			bufferPos_ = 0;
			outputBuffer_.assign(os.str());
			log_debug(0, "fd %d output buffer %s", fd_, outputBuffer_.c_str());
			cycle->disconnectBackEnd(easyBackEnds_);
		}
	}
	ssize_t n;
	while ((n = write(fd_, outputBuffer_.c_str() + bufferPos_, outputBuffer_.size() - bufferPos_)) > 0) {
		bufferPos_ += n;
		if (bufferPos_ == outputBuffer_.size()) break;
	}

	if (bufferPos_ == outputBuffer_.size()) {
		if (evset_) {
			evset_ = false;
			event_del(&event_);	
		}
		log_debug(0, "fd %d close", fd_);
		close(fd_);
	} else {
		if (n == -1) {
			if (errno != EAGAIN && errno != EINTR) {
				if (evset_) {
					evset_ = false;	
					event_del(&event_);
				}
				log_debug(0, "fd %d write", fd_);	
				log_debug(0, "fd %d close", fd_);
				close(fd_);
				return;
			}
		}

		if (!evset_) {
			evset_ = true;
			event_set(&event_, fd_, EV_WRITE | EV_PERSIST, TcpConnection::doSendWrap, this);
			event_add(&event_, NULL);
			log_debug(0, "fd %d add EV_WRITE", fd_);
		}
	}

	return;
}

bool Cycle::connectBackEnd(TcpConnection *tc, const UriMap &uriItems, std::map<CURL *, std::string> *easyBackEnds)
{
	total_ += uriItems.size();

	if (!checkEasyCurlPool(uriItems.size())) return false;

	for (UriMap::const_iterator ite = uriItems.begin(), end = uriItems.end(); ite != end; ++ite) {
		// TODO
		CURL *easy = easyCurlPool_.back();

		curl_easy_reset(easy);
		curl_easy_setopt(easy, CURLOPT_URL, (char*)ite->first.c_str());
		curl_easy_setopt(easy, CURLOPT_HEADERFUNCTION, backEndHeaderCallbackWrap);
		curl_easy_setopt(easy, CURLOPT_HEADERDATA, easy);
		curl_easy_setopt(easy, CURLOPT_WRITEFUNCTION, backEndCallbackWrap);
		curl_easy_setopt(easy, CURLOPT_WRITEDATA, easy);
		curl_easy_setopt(easy, CURLOPT_PRIVATE, tc);

		CURLMcode rc = curl_multi_add_handle(multi_, easy);
		if (rc != CURLM_OK) {
			log_error(0, "add easy #%p to connect #%p, %s", easy, tc, curl_multi_strerror(rc));	
			for (std::map<CURL *, std::string>::iterator jte = easyBackEnds->begin(), jend = easyBackEnds->end();
					jte != jend; ++jte) {
				easyCurlPool_.push_back(jte->first);
			}
			easyBackEnds->clear();
			return false;
		}

		easyCurlPool_.pop_back();
		backEndProtocolPool_[easy].reset(ite->second);
		easyBackEnds->insert(std::make_pair(easy, ite->first));

		log_debug(0, "add easy #%p to connect #%p", easy, tc);
	}

	return true;
}

void Cycle::disconnectBackEnd(const std::map<CURL *, std::string> &easyBackEnds) {
	for (std::map<CURL *, std::string>::const_iterator ite = easyBackEnds.begin(), end = easyBackEnds.end();
			ite != end; ++ite) {
		easyCurlPool_.push_back(ite->first);	
	}
	total_ -= easyBackEnds.size();
}

void Cycle::timerCallbackWrap(int /*fd*/, short /*kind*/, void *userp)
{
	Cycle *me = static_cast<Cycle *>(userp);
	me->timerCallback();
}

void Cycle::timerCallback()
{
	log_debug(0, "timer callback active %d total %d", active_, total_);

	CURLMcode rc;
	do {
		rc = curl_multi_socket_action(multi_, CURL_SOCKET_TIMEOUT, 0, &active_);	
	} while (rc == CURLM_CALL_MULTI_PERFORM);

	checkRunCount();
}

int Cycle::multiCallbackWrap(CURL *e, int fd, int action, void *userp, void *socketp) 
{
	Cycle *me = static_cast<Cycle *>(userp);
	HttpBackEnd *backEnd = static_cast<HttpBackEnd *>(socketp);
	return me->multiCallback(e, fd, action, backEnd);
}

int Cycle::multiCallback(CURL *e, int fd, int action, HttpBackEnd *backEnd)
{
#ifdef _DEBUG_
	const char *strAction[] = { "NONE", "IN", "OUT", "INOUT", "REMOVE" };
#endif

	TcpConnection *tc = 0;
	curl_easy_getinfo(e, CURLINFO_PRIVATE, &tc);

	assert(tc != 0);

	if (action == CURL_POLL_REMOVE) {
		log_debug(0, "fd %d#%p, e=%p, what=%s", fd, tc, e, strAction[action]);
		removeBackEnd(tc, backEnd);
	} else {
		if (backEnd) {
			log_debug(0, "fd %d#%p, e=%p, from %s to %s", fd, tc, e,
					     strAction[backEnd->action], strAction[action]);
			setBackEnd(e, fd, action, tc, backEnd);
		} else {
			log_debug(0, "fd %d#%p, e=%p, what=%s", fd, tc, e, strAction[action]);	
			addBackEnd(e, fd, action, tc);
		}
	} 
	return 0;
}

int Cycle::multiTimerCallbackWrap(CURLM * /* multi */, long ms, void *userp)
{
	Cycle *me = static_cast<Cycle *>(userp);
	return me->multiTimerCallback(ms);
}

int Cycle::multiTimerCallback(long ms)
{
	struct timeval timeout;

	timeout.tv_sec = ms / 1000;
	timeout.tv_usec = (ms % 1000) * 1000;

	log_debug(0, "multi #%p setting timetout to %ld ms\n", multi_, ms);
	evtimer_add(&timerEvent_, &timeout);

	return 0;
}

void Cycle::addBackEnd(CURL *e, int fd, int action, TcpConnection *tc)
{
	HttpBackEnd *backEnd = getBackEnd(tc, e);
	setBackEnd(e, fd, action, tc, backEnd);
	curl_multi_assign(multi_, fd, backEnd);
}

void Cycle::setBackEnd(CURL *e, int fd, int action, TcpConnection *tc, HttpBackEnd *backEnd)
{
	int kind = (action & CURL_POLL_IN ? EV_READ : 0) |
	           (action & CURL_POLL_OUT ? EV_WRITE : 0) |
			   EV_PERSIST;


	backEnd->fd = fd;
	backEnd->action = action;
	backEnd->easy = e;
	if (backEnd->evset) {
		event_del(&backEnd->ev);	
	}
	event_set(&backEnd->ev, backEnd->fd, kind, Cycle::eventCallbackWrap, tc);
	event_add(&backEnd->ev, NULL);
	backEnd->evset = true;
}

void Cycle::removeBackEnd(TcpConnection * /* tc */, HttpBackEnd *backEnd)
{
	if (backEnd) {
		if (backEnd->evset) {
			event_del(&backEnd->ev);	
		}
	}
}

void Cycle::eventCallbackWrap(int fd, short event, void *userp)
{
	TcpConnection *me = static_cast<TcpConnection *>(userp);
	me->cycle->eventCallback(fd, event, me);
}

void Cycle::eventCallback(int fd, short event, TcpConnection *tc)
{
	log_debug(0, "event callback");

	int action = (event & EV_READ ? CURL_CSELECT_IN : 0) |
		         (event & EV_WRITE ? CURL_CSELECT_OUT : 0);

	CURLMcode rc;
	do {
		rc = curl_multi_socket_action(multi_, fd, action, &active_);
	} while (rc == CURLM_CALL_MULTI_PERFORM);

	checkRunCount(tc);

	if (total_ == 0) {
		if (evtimer_pending(&timerEvent_, NULL)) {
			evtimer_del(&timerEvent_);	
		}
	}
}

void Cycle::checkRunCount(TcpConnection * /* tc */)
{

	CURL *easy;
	TcpConnection *easyTc;

	do {
		easy = 0;
		easyTc = 0;

		CURLMsg *msg;
		CURLcode rc;
		int msgsLeft;

		while ((msg = curl_multi_info_read(multi_, &msgsLeft))) {
			if (msg->msg == CURLMSG_DONE) {
				easy = msg->easy_handle;
				rc = msg->data.result;
				break;
			}
		}

		if (easy) {
			curl_easy_getinfo(easy, CURLINFO_PRIVATE, &easyTc);
			assert(easyTc != 0);
			curl_multi_remove_handle(multi_, easy);

			easyTc->descRunnings();
			if (easyTc->finished()) {
				easyTc->doSend();
			}

			log_debug(0, "remove easy #%p from connection #%p, left %d", easy, easyTc, active_);
		}
	} while (easy);

}

size_t Cycle::backEndHeaderCallbackWrap(void *ptr, size_t size, size_t n, void *userp)
{
	CURL *easy = static_cast<CURL *>(userp);
	TcpConnection *me = 0;
	curl_easy_getinfo(easy, CURLINFO_PRIVATE, &me);
	assert(me != 0);
	return me->cycle->backEndHeaderCallback(static_cast<const char*>(ptr), size * n, easy);
}

size_t Cycle::backEndHeaderCallback(const char *buffer, size_t size, CURL *easy)
{
	long httpCode;
	if (curl_easy_getinfo(easy, CURLINFO_RESPONSE_CODE, &httpCode) == CURLE_OK) {
		if (httpCode != 200) {
			char *url;
			if (curl_easy_getinfo(easy, CURLINFO_EFFECTIVE_URL, &url) != CURLE_OK) {
				url = "unknow url";	
			}
			log_error(0, "request %s error, http code %ld", url, httpCode);
			backEndPool_[easy].state = HttpBackEnd::Not200;
			return -1;
		}
	}
	assert(backEndProtocolPool_.count(easy) == 1);
	backEndProtocolPool_[easy].inputHeaderAppend(buffer, size);
	return size;
}


size_t Cycle::backEndCallbackWrap(void *ptr, size_t size, size_t n, void *userp) 
{
	CURL *easy = static_cast<CURL *>(userp);
	TcpConnection *me = 0;
	curl_easy_getinfo(easy, CURLINFO_PRIVATE, &me);

	return me->cycle->backEndCallback(static_cast<const char*>(ptr), size * n, easy);
}

size_t Cycle::backEndCallback(const char *buffer, size_t size, CURL *easy) {
	std::map<CURL *, HttpProtocol>::iterator ite = backEndProtocolPool_.find(easy);
	if (ite == backEndProtocolPool_.end()) {
		return 0;
	}
	ite->second.inputAppend(buffer, size);
	return size;
}

void TcpConnection::reset(int fd)
{
	fd_ = fd;
	evset_ = false;
	easyBackEnds_.clear();
	protocol_.reset();
}

bool Cycle::checkEasyCurlPool(size_t n)
{
	while (easyCurlPool_.size() < n) {
		CURL *easy = curl_easy_init();	
		if (!easy) return false;
		easyCurlPool_.push_back(easy);
		backEndProtocolPool_[easy] = HttpProtocol();
		backEndPool_[easy] = HttpBackEnd();
	}	
	return true;
}

Cycle::Cycle() : multi_(0), active_(0), total_(0)
{
	listening_ = 0;
	rankerList_ = 0;

	currentCycle = this;
	updateTime();
}

Cycle::~Cycle()
{
	if (multi_) curl_multi_cleanup(multi_);

	for (std::vector<CURL *>::iterator ite = easyCurlPool_.begin(), end = easyCurlPool_.end(); ite != end; ++ite) {
		curl_easy_cleanup(*ite);
	}
}

bool Cycle::prepare(int maxfd)
{
	if (listening_ == 0 ||
        rankerList_ == 0) {
		log_error(0, "TcpListening OR std::list<Ranker *> not setted");	
		return false;
	}

	struct event_base *eventBase = event_init();
	if (!eventBase) return false;

	connectionPool_.resize(maxfd);
	
	event_base_set(eventBase, &listening_->ev);
	event_add(&listening_->ev, NULL);

	evtimer_set(&timerEvent_, Cycle::timerCallbackWrap, this);

	multi_ = curl_multi_init();
	if (!multi_) return false;

	curl_multi_setopt(multi_, CURLMOPT_SOCKETFUNCTION, Cycle::multiCallbackWrap);
	curl_multi_setopt(multi_, CURLMOPT_SOCKETDATA, this);
	curl_multi_setopt(multi_, CURLMOPT_TIMERFUNCTION, Cycle::multiTimerCallbackWrap);
	curl_multi_setopt(multi_, CURLMOPT_TIMERDATA, this);

	easyCurlPool_.reserve(maxfd_);
	for (int i = 0; i < maxfd_; ++i) {
		CURL *easy = curl_easy_init();
		if (!easy) return false;
		backEndPool_[easy] = HttpBackEnd();
		backEndProtocolPool_[easy] = HttpProtocol();
	}

	maxfd_ = maxfd;

	return true;
}

bool Cycle::loop()
{
	event_dispatch();	
	log_debug(0, "loop exit\n");

	return true;
}

TcpConnection *Cycle::getConnection(int fd)
{
	if (fd >= maxfd_) return 0;

	TcpConnection *tc = &connectionPool_[fd];
	tc->cycle = this;

	return tc;
}

HttpBackEnd *Cycle::getBackEnd(TcpConnection *tc, CURL *e) {
	HttpBackEnd *backEnd = &backEndPool_[e];
	backEnd->tc = tc;
	backEnd->evset = false;
	backEnd->easy = e;
	backEnd->state = HttpBackEnd::Ok;
	return backEnd;
}

} // namespace Cc

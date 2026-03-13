#include "Webserv.hpp"

/*******************************************************************************
*						TIMEOUT
*******************************************************************************/

void Webserv::_handleCgiTimeout(std::time_t &now)
{
	for (size_t j = 0; j < this->_servers.size(); j++)
	{
		for (size_t i = 0; i < this->_servers[j].getCgiVec().size(); i++)
		{
			CGI	&cgi = this->_servers[j].getCgiVec()[i];
			if (this->_clientMap[cgi.getClientFD()].client.getCgiResponseState() == 1 && cgi.getStartTimestamp() != 0 && now - cgi.getStartTimestamp() > CGI_TIMEOUT)
			{
			   std::cout << "CGI timed out" << std::endl;
				this->_clientMap[cgi.getClientFD()].client.getResponse().buildErrorResponse(503, &this->_servers[j], &cgi.getLocation());
				//make sure no trailling FDs forgotten in epoll
				epoll_ctl(this->_epollFd, EPOLL_CTL_DEL, cgi.getWriteFD(), NULL);
				epoll_ctl(this->_epollFd, EPOLL_CTL_DEL, cgi.getClientFD(), NULL);
				epoll_ctl(this->_epollFd, EPOLL_CTL_DEL, cgi.getReadFD(), NULL);
                _modifyEpoll(EPOLLOUT, EPOLL_CTL_ADD, cgi.getClientFD());
				_destroyCGI(cgi, this->_servers[j]);
			}
		}
	}
}


void    Webserv::_handleTimeouts(void)
{
	std::time_t	now;
	std::time_t firstCo;
	std::time_t	recvStamp;
	std::time_t	sendStamp;
	bool		reqFlag;
	bool		responseFlag;	   
	
	time(&now);
	if (this->_clientMap.empty())
		return ;
	for (std::map<int, t_connection>::iterator it = this->_clientMap.begin(); it != this->_clientMap.end(); it++)
	{
		Client	&client = it->second.client;
		firstCo = client.getFirstCoTimestamp();
		recvStamp = client.getRequest().getRecvTimestamp();
		sendStamp = client.getResponse().getSendTimestamp();
		reqFlag = client.getRequest().getReqFlag();
		responseFlag = client.getResponse().getRespFlag();

		//recv/send timeouts
		//check if 1) request/response complete 2) timestamp initialized === first receive/send happend 3) timeout status
		if ((firstCo != 0 && now - firstCo > FIRST_CONNEXION_TIMEOUT) || (!reqFlag && recvStamp != 0 && now - recvStamp > QUERY_TIMEOUT))
		{
			std::cout << "request timed out" << std::endl;
			client.getResponse().buildErrorResponse(408, it->second.server, NULL);
            _modifyEpoll(EPOLLOUT, EPOLL_CTL_MOD, client.getFd());
		}
		if (!responseFlag && sendStamp != 0 && now - sendStamp > QUERY_TIMEOUT)
		{
			std::cout << "response timed out" << std::endl;
			_closeClient(client.getFd());
		}
	}
	//cgi timeout
	this->_handleCgiTimeout(now);	
}

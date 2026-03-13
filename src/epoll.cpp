#include "Webserv.hpp"

extern volatile sig_atomic_t	g_signum;

/*******************************************************************************
*						EPOLL LOOP
*******************************************************************************/

void	Webserv::launchServer(void)
{
	int					readyFds;
	struct epoll_event	readyEvents[200];
	long				idx = 0;

	this->_epollFd = this->_setupEpoll();
	while (1)
	{
		// std::cout << "\nWaiting for connections" << std::endl;
		readyFds = epoll_wait(_epollFd, readyEvents, 200, 2500);
		if (g_signum == SIGINT)
		{
			_cleanExit();
			break;
		}
		for (int i = 0; i < readyFds; i++)
		{
			// activityNotif(readyEvents[i]);

			if (_isListenSocket(readyEvents[i].data.fd))
				_newClient(readyEvents[i].data.fd);
			else if ((idx = _isCgiFd(readyEvents[i].data.fd)) != -1)
			{
				int servIdx = idx >> 16;
				int cgiIdx = idx & 0xFFFF;
				if (this->_servers[servIdx].getCgiVec()[cgiIdx].getErrorFD() == readyEvents[i].data.fd)
					_handleErrorPipe(this->_servers[servIdx].getCgiVec()[cgiIdx]);
				else if (readyEvents[i].events & EPOLLOUT)
					_handleCgiInput(this->_servers[servIdx].getCgiVec()[cgiIdx], this->_servers[servIdx]); //fd is CGI.writeFD
				else
					_handleCgiOutput(this->_servers[servIdx].getCgiVec()[cgiIdx], this->_servers[servIdx]); //fd is CGI.readFD
			}
			else
			{
				if (readyEvents[i].events & EPOLLIN)
					_handleRequest(readyEvents[i].data.fd);
				else if (readyEvents[i].events & EPOLLOUT)
					_handleResponse(readyEvents[i].data.fd);
				else
					_closeClient(readyEvents[i].data.fd);
			}
		}
		_handleTimeouts();
	}
}


/*******************************************************************************
*						EXIT
*******************************************************************************/

void	Webserv::_cleanExit(void)
{
	for (size_t j = 0; j < this->_servers.size(); j++)
	{
		for (size_t i = 0; i < this->_servers[j].getCgiVec().size(); i++)
		{
			_destroyCGI(this->_servers[j].getCgiVec()[i], this->_servers[j]);
		}
	}
	for (std::map<int, Server*>::iterator it = this->_serverMap.begin(); it !=  this->_serverMap.end(); it++)
	{
		close(it->first);
	}
	this->_serverMap.clear();
	for (std::map<int, t_connection>::iterator it = this->_clientMap.begin(); it != this->_clientMap.end(); it++)
	{
		close(it->first);
	}
	this->_clientMap.clear();
	close(this->_epollFd);
}


/*******************************************************************************
*						UTILS
*******************************************************************************/

void	Webserv::_activityNotif(struct epoll_event	readyEvent)
{
	if (_isListenSocket(readyEvent.data.fd))
		std::cout << "ListenSocket (" << readyEvent.data.fd << ") is ready for ";
	else if (_isCgiFd(readyEvent.data.fd) != -1)
		std::cout << "Pipe (" << readyEvent.data.fd << ") is ready for ";
	else
		std::cout << "Client (" << readyEvent.data.fd << ") is ready for ";
	
	if (readyEvent.events == 1)
		std::cout << "EPOLLIN" << std::endl;
	else if (readyEvent.events == 4)
		std::cout << "EPOLLOUT" << std::endl;
	else
		std::cout << readyEvent.events << std::endl;
}


bool	Webserv::_isListenSocket(int fd) const
{
	if (_serverMap.find(fd) != _serverMap.end())
		return true;
	return false;
}


long  Webserv::_isCgiFd(int fd) //return a pair to identify server and CGI OR a bitshifted number ??? like return codes and error codes
{
	for (size_t j = 0; j < this->_servers.size(); j++)
	{
		for (size_t i = 0; i < this->_servers[j].getCgiVec().size(); i++)
		{
			if (fd == this->_servers[j].getCgiVec()[i].getErrorFD() || fd == this->_servers[j].getCgiVec()[i].getReadFD() || fd == this->_servers[j].getCgiVec()[i].getWriteFD())
				return ((static_cast<int>(j) << 16) | static_cast<int>(i));
		}
	}
	return (-1);
}
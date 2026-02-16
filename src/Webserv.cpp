#include "../include/Webserv.hpp"
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <cstdlib>
#include <stdexcept>
#include <cerrno>
#include <unistd.h>
#include <cstring>
#include <sys/epoll.h>
#include <iostream>
#include <fcntl.h>

/*******************************************************************************
*						CTOR/DTOR
*******************************************************************************/

Webserv::Webserv(void): _numServers(1), _servers(new Server[1]) {}

Webserv::Webserv(unsigned int numServers):
	_numServers(numServers),
	_servers(new Server[numServers]) 
{}

Webserv::Webserv(Webserv const &src):
	_numServers(src._numServers),
	_servers(new Server[src._numServers])
{
	for (int i = 0; i < src._numServers; i++)
	{
		_servers[i] = src._servers[i];
	}
	this->_serverMap = src._serverMap;
}

Webserv	&Webserv::operator=(Webserv const &rhs)
{
	if (this != &rhs)
	{
		delete[] this->_servers;
		this->_servers = new Server[rhs._numServers];
		this->_numServers = rhs._numServers;
		for (int i = 0; i < rhs._numServers; i++)
		this->_servers[i] = rhs._servers[i];
		this->_serverMap = rhs._serverMap;
	}
	return (*this);
}

Webserv::~Webserv(void){}


/*******************************************************************************
*						INIT
*******************************************************************************/

void	Webserv::setServer(Server server, int pos)
{
	this->_servers[pos] = server;
}

void	Webserv::openSockets(void)
{
	struct	addrinfo	hints;
	struct addrinfo 	*ptr;
	int					yes = 1;

	std::memset(&hints, 0, sizeof(hints));
	hints.ai_family = AF_INET;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_flags = AI_PASSIVE;
	for (int i = 0; i < this->_numServers; i++)
	{
		for (int j = 0; j < this->_servers[i].getNumSockets(); j++)
		{
			struct	addrinfo	*res;
			int					gaiError;
			int					sFd;

			if ((gaiError = getaddrinfo(this->_servers[i].getSocket(j).getIpAddr().c_str(),
					this->_servers[i].getSocket(j).getPort().c_str(), &hints, &res)) != 0)
				throw(std::runtime_error(gai_strerror(gaiError)));
			for (ptr = res; ptr != NULL; ptr = ptr->ai_next)
			{
				if ((sFd = socket(ptr->ai_family, ptr->ai_socktype, ptr->ai_protocol)) == -1)
					continue;
				setsockopt(sFd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(yes));
				if (bind(sFd, ptr->ai_addr, ptr->ai_addrlen) != 0)
				{
					close(sFd);
					continue;
				}
				break;
			}
			freeaddrinfo(res);
			if (ptr == NULL)
				throw(std::runtime_error(std::strerror(errno)));
			if (listen(sFd, 15) == -1)
				throw(std::runtime_error(std::strerror(errno)));
			std::cout << sFd << "Listening at " << this->_servers[i].getSocket(j).getIpAddr() << " on port " << this->_servers[i].getSocket(j).getPort() << std::endl;
			this->_serverMap.insert(std::pair<int, Server>(sFd, this->_servers[i]));
		}
	}
}

int	Webserv::setupEpoll(void) const
{
	int	epollFd;

	if ((epollFd = epoll_create(1)) == -1)
		throw(std::runtime_error(std::strerror(errno)));
	for (std::map<int, Server>::const_iterator it = this->_serverMap.begin(); it != this->_serverMap.end(); it++)
	{
		struct epoll_event	event;

		event.events = EPOLLIN;
		event.data.fd = it->first;
		if (epoll_ctl(epollFd, EPOLL_CTL_ADD, it->first, &event) == -1)
			throw(std::runtime_error(std::strerror(errno)));
		std::cout << "Added fd: " << it->first << " to the epoll instance" << std::endl;
	}
	return (epollFd);
}

void	Webserv::launchServer(void)
{
	int					readyFds;
	struct epoll_event	readyEvents[10];

	this->_epollFd = this->setupEpoll();
	while (1)
	{
		std::cout << "Waiting for connections" << std::endl;
		readyFds = epoll_wait(_epollFd, readyEvents, 10, -1);
		for (int i  = 0; i < readyFds; i++)
		{
			std::cout << "FD: " << readyEvents[i].data.fd << " is ready for " << readyEvents[i].events << std::endl;

			if (isListenSocket(readyEvents[i].data.fd))
				newClient(readyEvents[i].data.fd);
			else
			{
				if (readyEvents[i].events & EPOLLIN)
					handleRequest(readyEvents[i].data.fd);
				else if (readyEvents[i].events & EPOLLOUT)
					handleResponse(readyEvents[i].data.fd);
				else
					closeClient(readyEvents[i].data.fd);
			}
		}
	}
}


/*******************************************************************************
*						CLIENT HELPERS
*******************************************************************************/

bool	Webserv::isListenSocket(int fd) const
{
	if (_serverMap.find(fd) != _serverMap.end())
		return true;
	return false;
}

void	Webserv::newClient(int listenFd)
{
	struct sockaddr	clientAddr;
	socklen_t		addrSize = sizeof(clientAddr);

	int	clientFd = accept(listenFd, &clientAddr, &addrSize);
	if (clientFd == -1)
		throw(std::runtime_error(std::strerror(errno)));

    int flags = fcntl(clientFd, F_GETFL, 0);
	if (flags == -1 || fcntl(clientFd, F_SETFL, flags | O_NONBLOCK) == -1)
    {
        close(clientFd);
		throw(std::runtime_error(std::strerror(errno)));
    }

    _clientMap[clientFd] = new Client(clientFd);

	struct epoll_event event;
    event.events = EPOLLIN;
    event.data.fd = clientFd;
	 if (epoll_ctl(_epollFd, EPOLL_CTL_ADD, clientFd, &event) == -1)
    {
        delete _clientMap[clientFd];
        _clientMap.erase(clientFd);
        close(clientFd);
		throw(std::runtime_error(std::strerror(errno)));
    }

	std::cout << "New client with fd: " << clientFd << std::endl;
}

void	Webserv::handleRequest(int clientFd)
{

}

void	Webserv::handleResponse(int clientFd)
{

}

void	Webserv::closeClient(int clientFd)
{
	epoll_ctl(_epollFd, EPOLL_CTL_DEL, clientFd, NULL);
    close(clientFd);
    delete _clientMap[clientFd];
    _clientMap.erase(clientFd);
    std::cout << "Client disconnected with fd: " << clientFd << std::endl;
}

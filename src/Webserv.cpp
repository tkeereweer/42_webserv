#include "../include/Webserv.hpp"
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <cstdlib>
#include <stdexcept>
#include <cerrno>
#include <unistd.h>
#include <cstring>

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
	delete[] this->_servers;
	this->_servers = new Server[rhs._numServers];
	this->_numServers = rhs._numServers;
	for (int i = 0; i < rhs._numServers; i++)
		this->_servers[i] = rhs._servers[i];
	this->_serverMap = rhs._serverMap;
	return (*this);
}

Webserv::~Webserv(void)
{
	delete[] this->_servers;
}

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
		for (int j = 0; j < this->_servers[i].getNumSockets(); i++)
		{
			struct	addrinfo	*res;
			int					sfd;

			if (getaddrinfo("127.0.0.1", this->_servers[i].getSocket(j).getPort().c_str(), &hints, &res) != 0)
				throw(std::runtime_error(std::strerror(errno)));
			for (ptr = res; ptr != NULL; ptr = ptr->ai_next)
			{
				if ((sfd = socket(ptr->ai_family, ptr->ai_socktype, ptr->ai_protocol)) == -1)
					continue;
				setsockopt(sfd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(yes));
				if (bind(sfd, ptr->ai_addr, ptr->ai_addrlen) != 0)
				{
					close(sfd);
					continue;
				}
				break;
			}
			freeaddrinfo(res);
			if (ptr == NULL)
				throw(std::runtime_error(std::strerror(errno)));
			if (listen(sfd, 15) == -1)
				throw(std::runtime_error(std::strerror(errno)));
			this->_serverMap.insert(std::pair<int, Server>(sfd, this->_servers[i]));
		}
	}
}

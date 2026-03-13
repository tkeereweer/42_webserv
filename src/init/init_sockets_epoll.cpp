#include "Webserv.hpp"

extern int						g_sigPipe[2];

/*******************************************************************************
*						CONFIG PARSING
*******************************************************************************/

void	Webserv::getConfig(char const *filepath)
{
	std::string	content = _openFile(filepath);
	std::list<t_conf_token>	tokens = _lexConfigFile(content);
	_parseConfTokens(tokens);
	if (this->_servers.size() < 1)
		throw(std::runtime_error("No server in config file"));
}

/*******************************************************************************
*						EPOLL INIT
*******************************************************************************/

void	Webserv::openSockets(void)
{
	struct	addrinfo	hints;
	struct addrinfo 	*ptr;
	int					yes = 1;
	int					N_LISTEN = 250;

	std::memset(&hints, 0, sizeof(hints));
	hints.ai_family = AF_INET;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_flags = AI_PASSIVE;
	for (unsigned long i = 0; i < this->_servers.size(); i++)
	{
		for (std::vector<t_socket>::iterator it = this->_servers[i].getSockets().begin();
			it != this->_servers[i].getSockets().end(); ++it)
		{
			struct	addrinfo	*res;
			int					gaiError;
			int					sFd;

			if ((gaiError = getaddrinfo(it->ipAddr.c_str(), it->port.c_str(), &hints, &res)) != 0)
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
			if (listen(sFd, N_LISTEN) == -1)
				throw(std::runtime_error(std::strerror(errno)));
			std::cout << "ListenSocket ("<< sFd << ") is listening " << it->ipAddr << " on port:" << it->port << std::endl;
			this->_serverMap.insert(std::pair<int, Server*>(sFd, &(this->_servers[i])));
		}
	}
}


int	Webserv::_setupEpoll(void) const
{
	int					epollFd;
	struct epoll_event	event;

	event.events = EPOLLIN;
	if ((epollFd = epoll_create(1)) == -1)
		throw(std::runtime_error(std::strerror(errno)));
	if (epoll_ctl(epollFd, EPOLL_CTL_ADD, g_sigPipe[0], &event) == -1)
		throw(std::runtime_error(std::strerror(errno)));
	for (std::map<int, Server*>::const_iterator it = this->_serverMap.begin(); it != this->_serverMap.end(); it++)
	{
		event.data.fd = it->first;
		if (epoll_ctl(epollFd, EPOLL_CTL_ADD, it->first, &event) == -1)
			throw(std::runtime_error(std::strerror(errno)));
		std::cout << "ListenSocket (" << it->first << ") added to epoll" << std::endl;
	}
	return (epollFd);
}

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

Webserv::Webserv(void) {}

Webserv::Webserv(Webserv const &src):
	_servers(src._servers),
	_serverMap(src._serverMap),
	_clientMap(src._clientMap),
	_epollFd(src._epollFd)
{}

Webserv	&Webserv::operator=(Webserv const &rhs)
{
	if (this != &rhs)
	{
		this->_servers = rhs._servers;
		this->_serverMap = rhs._serverMap;
		this->_clientMap = rhs._clientMap;
		this->_epollFd = rhs._epollFd; // close old epollFd ??
	}
	return (*this);
}

Webserv::~Webserv(void) {}

/*******************************************************************************
*						GET/SET
*******************************************************************************/

std::vector<Server>	&Webserv::getServers(void)
{
	return (this->_servers);
}

void	Webserv::addServer(Server server)
{
	this->_servers.push_back(server);
}

/*******************************************************************************
*						INIT
*******************************************************************************/

std::ostream	&operator<<(std::ostream &o, Webserv &input)
{
	for (unsigned int i = 0; i < input.getServers().size(); i++)
	{
		o << "server {" << std::endl;
		o << "\tserver_name " << input.getServers()[i].getName() << ";" << std::endl;
		o << "\tclient_max_body_size " << input.getServers()[i].getMaxBody() << ";" << std::endl;
		o << "\troot " << input.getServers()[i].getServerRoot() << ";" << std::endl;
		for (unsigned int j = 0; j < input.getServers()[i].getSockets().size(); j++)
			o << "\tlisten " << input.getServers()[i].getSockets()[j].ipAddr << "/" << input.getServers()[i].getSockets()[j].port << ";" << std::endl;
		for (unsigned int j = 0; j < input.getServers()[i].getLocations().size(); j++)
		{
			o << "\tlocation " << input.getServers()[i].getLocations()[j].getPath() << " {" << std::endl;
			o << "\t\troot " << input.getServers()[i].getLocations()[j].getLocRoot() << ";" << std::endl;
			o << "\t\tlimit_except GET:" << input.getServers()[i].getLocations()[j].getAcceptGET() << " POST:" << input.getServers()[i].getLocations()[j].getAcceptPOST() << " DELETE:" << input.getServers()[i].getLocations()[j].getAcceptDELETE() << ";" << std::endl;
			o << "\t\tautoindex " << input.getServers()[i].getLocations()[j].getAutoIndex() << ";" << std::endl;
			o << "\t\tindex " << input.getServers()[i].getLocations()[j].getIndex() << ";" << std::endl;
			o << "\t\tupload_store " << input.getServers()[i].getLocations()[j].getUploadStore() << ";" << std::endl;
			for (std::map<int, std::string>::iterator it = input.getServers()[i].getLocations()[j].getErrorPages().begin(); it != input.getServers()[i].getLocations()[j].getErrorPages().end(); ++it)
				o << "\t\terror_page " << it->first << " " << it->second << ";" << std::endl;
			o << "\t\treturn " << input.getServers()[i].getLocations()[j].getRedir().first << " " << input.getServers()[i].getLocations()[j].getRedir().second << ";" << std::endl;
			o << "\t}" << std::endl;
		}
		o << "}" << std::endl;
	}
	return (o);
}

void	Webserv::parseConfTokens(std::list<t_conf_token>::iterator &token)
{

	if (token->type == WORD && token->value == "server")
	{
		Server	server;
		token++;
		if (token->type == OPEN_CURLY)
		{
			token++;
			parseServerBlock(token, server);
			this->addServer(server);
		}
		else
			throw(std::runtime_error("Syntax error in config file"));
	}
	else
		throw(std::runtime_error("Config file does not start with a server block"));
}

void	Webserv::getConfig(char const *filepath)
{
	std::string	content = openFile(filepath);
	std::list<t_conf_token> tokens = lexConfigFile(content);
	printConfTokens(tokens);
	std::list<t_conf_token>::iterator	start = tokens.begin();
	parseConfTokens(start);
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
			if (listen(sFd, 15) == -1)
				throw(std::runtime_error(std::strerror(errno)));
			std::cout << sFd << "Listening at " << it->ipAddr << " on port " << it->port << std::endl;
			this->_serverMap.insert(std::pair<int, Server*>(sFd, &(this->_servers[i])));
		}
	}
}

int	Webserv::setupEpoll(void) const
{
	int	epollFd;

	if ((epollFd = epoll_create(1)) == -1)
		throw(std::runtime_error(std::strerror(errno)));
	for (std::map<int, Server*>::const_iterator it = this->_serverMap.begin(); it != this->_serverMap.end(); it++)
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
				if (readyEvents[i].events & (EPOLLERR | EPOLLHUP))
					closeClient(readyEvents[i].data.fd);
				else if (readyEvents[i].events & EPOLLIN)
					handleRequest(readyEvents[i].data.fd);
				else if (readyEvents[i].events & EPOLLOUT)
					handleResponse(readyEvents[i].data.fd);
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

void	Webserv::testPrint(int clientFd)
{
	std::string request = _clientMap[clientFd]->getRequest();
	size_t nl = request.find('\n');

	std::string print =request.substr(0, nl);
	std::string rest = request.substr(nl + 1);

	_clientMap[clientFd]->setRequest(rest);
	std::cout << print << std::endl;
}

void	Webserv::handleRequest(int clientFd)
{
	char	buffer[1024];

	ssize_t bytesRead = recv(clientFd, buffer, sizeof(buffer) - 1, 0);

 	if (bytesRead > 0)
    {
        buffer[bytesRead] = '\0';
        _clientMap[clientFd]->appendRequest(std::string(buffer, bytesRead));

		if ((_clientMap[clientFd]->getRequest().find('\n')) != std::string::npos)
			testPrint(clientFd);
	}

	else if (bytesRead == 0)
        closeClient(clientFd);
	
	else 
	{
		if (errno == EAGAIN || errno == EWOULDBLOCK) 
			return;
		else
			closeClient(clientFd);
	}
}

void	Webserv::handleResponse(int clientFd)
{
	(void)clientFd;
	// load into kernel buffer
	// track how much went in and store the rest
	// repeat for whole response
}

void	Webserv::closeClient(int clientFd)
{
	epoll_ctl(_epollFd, EPOLL_CTL_DEL, clientFd, NULL);
    close(clientFd);
    delete _clientMap[clientFd];
    _clientMap.erase(clientFd);
    std::cout << "Client disconnected with fd: " << clientFd << std::endl;
}

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
#include <fstream>
#include <sstream>

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

std::ostream	&operator<<(std::ostream &o, Webserv &input)
{
	for (unsigned int i = 0; i < input.getServers().size(); i++)
	{
		o << "server {" << std::endl;
		o << "\tserver_name " << input.getServers()[i].getName() << ";" << std::endl;
		o << "\troot " << input.getServers()[i].getRoot() << ";" << std::endl;
		o << "\tlimit_except GET:" << input.getServers()[i].getAcceptGET() << " POST:" << input.getServers()[i].getAcceptPOST() << " DELETE:" << input.getServers()[i].getAcceptDELETE() << ";" << std::endl;
		o << "\tautoindex " << input.getServers()[i].getAutoIndex() << ";" << std::endl;
		o << "\tindex " << input.getServers()[i].getIndex() << ";" << std::endl;
		o << "\tclient_max_body_size " << input.getServers()[i].getMaxBody() << ";" << std::endl;
		for (unsigned int j = 0; j < input.getServers()[i].getSockets().size(); j++)
			o << "\tlisten " << input.getServers()[i].getSockets()[j].ipAddr << "/" << input.getServers()[i].getSockets()[j].port << ";" << std::endl;
		for (std::map<int, std::string>::iterator it = input.getServers()[i].getErrorPages().begin(); it != input.getServers()[i].getErrorPages().end(); ++it)
				o << "\terror_page " << it->first << " " << it->second << ";" << std::endl;
			o << "\treturn " << input.getServers()[i].getRedir().first << " " << input.getServers()[i].getRedir().second << ";" << std::endl;
		for (unsigned int j = 0; j < input.getServers()[i].getLocations().size(); j++)
		{
			o << "\tlocation " << input.getServers()[i].getLocations()[j].getPath() << " {" << std::endl;
			o << "\t\troot " << input.getServers()[i].getLocations()[j].getRoot() << ";" << std::endl;
			o << "\t\tlimit_except GET:" << input.getServers()[i].getLocations()[j].getAcceptGET() << " POST:" << input.getServers()[i].getLocations()[j].getAcceptPOST() << " DELETE:" << input.getServers()[i].getLocations()[j].getAcceptDELETE() << ";" << std::endl;
			o << "\t\tautoindex " << input.getServers()[i].getLocations()[j].getAutoIndex() << ";" << std::endl;
			o << "\t\tindex " << input.getServers()[i].getLocations()[j].getIndex() << ";" << std::endl;
			o << "\t\tclient_max_body_size " << input.getServers()[i].getLocations()[j].getMaxBody() << ";" << std::endl;
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

/*******************************************************************************
*						GET/SET
*******************************************************************************/

std::vector<Server>	&Webserv::getServers(void)
{
	return (this->_servers);
}

std::map<int, Server*>	&Webserv::getServerMap(void)
{
	return (this->_serverMap);
}

void	Webserv::addServer(Server server)
{
	this->_servers.push_back(server);
}

/*******************************************************************************
*						CONFIG PARSING
*******************************************************************************/

void	Webserv::getConfig(char const *filepath)
{
	std::string	content = openFile(filepath);
	std::list<t_conf_token>	tokens = lexConfigFile(content);
	// printConfTokens(tokens);
	parseConfTokens(tokens);
	if (this->_servers.size() < 1)
		throw(std::runtime_error("No server in config file"));
}

/*******************************************************************************
*						INIT
*******************************************************************************/

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
			std::cout << "Listening Socket with fd: "<< sFd << " is listening at " << it->ipAddr << " on port " << it->port << std::endl;
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
		std::cout << "Listening Socket with fd: " << it->first << " added to the epoll" << std::endl;
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
		std::cout << "\nWaiting for connections" << std::endl;
		readyFds = epoll_wait(_epollFd, readyEvents, 10, -1);
		for (int i  = 0; i < readyFds; i++)
		{
			std::cout << "fd: " << readyEvents[i].data.fd << " has activity!" << std::endl;

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
		handleTimeouts();
	}
	handleTimeouts(); //for the case where epoll_wait times out but still hanging requests
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

	_clientMap[clientFd].client = Client(clientFd); // operator [] adds new entry if key does not exist yet
	_clientMap[clientFd].server = _serverMap[listenFd];

	struct epoll_event event;
	event.events = EPOLLIN;
	event.data.fd = clientFd;
	if (epoll_ctl(_epollFd, EPOLL_CTL_ADD, clientFd, &event) == -1)
	{
		_clientMap.erase(clientFd);
		close(clientFd);
		throw(std::runtime_error(std::strerror(errno)));
	}

	std::cout << "New client with fd: " << clientFd << std::endl;
}

void	Webserv::testPrint(int clientFd, Client &client)
{
	std::cout << client.getRequest() << std::endl;
	std::cout << "~~~~~~ end request ~~~~~~~" << std::endl;
	_clientMap[clientFd].client.setResponse("HTTP/1.0 200 OK");
	_clientMap[clientFd].client.clearReadBuffer();
}

void	Webserv::handleRequest(int clientFd)
{
	Client&		    client = _clientMap[clientFd].client;
	char		    buffer[1024];
	struct timeval	now;

	ssize_t bytesRead = recv(clientFd, buffer, sizeof(buffer) - 1, 0);
	gettimeofday(&now, NULL);
	client.getRequest().setRecvTimestamp(now);
	int	lexReturn = -2;

 	if (bytesRead > 0)
	{
		buffer[bytesRead] = '\0';
		client.appendReadBuffer(std::string(buffer, bytesRead));//data string
		try
		{
			lexReturn = client.getRequest().lexRawData(client.getReadBuffer());
		}
		catch(const std::exception& e)
		{
			std::cerr << e.what() << '\n';
			return (closeClient(clientFd));
		}
		if (lexReturn > 0) //write in tempfile logic
		{
			std::ofstream tmpFile(client.getRequest().getBodyFilename().c_str());
			tmpFile.write(client.getReadBuffer().c_str(), client.getReadBuffer().size());
			if (lexReturn - client.getReadBuffer().size() <= 0)
				lexReturn = -1;
		}
		if (lexReturn == -1)
		{
			std::cout << "~~~~~ request successfully received ! content: ~~~~~~" << std::endl;
			testPrint(clientFd, client); //put request dipsatcher here, build body here
			struct epoll_event event;
			event.events = EPOLLOUT;
			event.data.fd = clientFd;
			if (epoll_ctl(_epollFd, EPOLL_CTL_MOD, clientFd, &event) == -1)
			{
				_clientMap.erase(clientFd);
				close(clientFd);
				throw(std::runtime_error(std::strerror(errno)));
			}
		}
	}
	else
		closeClient(clientFd);
}

void	Webserv::handleResponse(int clientFd)
{
	Client&		client = _clientMap[clientFd].client;
	const char*	ptr = client.getResponse().c_str() + client.getBytesSent();
	size_t		remaining = client.getResponse().size() - client.getBytesSent();

	ssize_t bytesSentNow = send(clientFd, ptr, remaining, 0);

	if (bytesSentNow > 0)
	{
		client.addBytesSent(bytesSentNow);
		if (client.getBytesSent() == client.getResponse().size() )
			closeClient(clientFd);
	}
	else
		closeClient(clientFd);
}

void	Webserv::closeClient(int clientFd)
{
	epoll_ctl(_epollFd, EPOLL_CTL_DEL, clientFd, NULL);
	_clientMap.erase(clientFd);
	close(clientFd);
	std::cout << "Client disconnected with fd: " << clientFd << std::endl;
}

//t2 - t1
int	getTimeDiff(timeval t1, timeval t2)
{
    long dSeconds = t2.tv_sec  - t1.tv_sec;
    long dUseconds = t2.tv_usec - t1.tv_usec;

    return ((dSeconds) * 1000 + dUseconds / 1000.0) + 0.5; //+0.5 is a rounding technique to be sure we round the nearest integer.
}

void    Webserv::handleTimeouts(void)
{
	struct timeval	now;
	struct timeval	recvStamp;
	// struct timeval	sendStamp;
	bool			reqFlag;
	// bool			responseFlag;
	
	gettimeofday(&now, NULL);
	for (std::map<int, t_connection>::iterator it = this->_clientMap.begin(); it != this->_clientMap.end(); it++)
	{
		recvStamp = it->second.client.getRequest().getRecvTimestamp();
		// sendStamp = it->second.client.getRequest().getSendTimestamp();
		reqFlag = it->second.client.getRequest().getReqFlag();
		// responseFlag = it->client.getResponse().getRespFlag();

		//check if 1) request/response complete 2) timestamp initialized === first receive/send happend 3) timeout status
		if (!reqFlag && (recvStamp.tv_sec != 0 || recvStamp.tv_usec != 0) && getTimeDiff(recvStamp, now) > QUERY_TIMEOUT)
		{
			std::cout << "error 411 on client " << it->second.client.getFd() << ": request timeout" << std::endl;
			closeClient(it->second.client.getFd());
		}
		// if (!responseFlag && (sendStamp.tv_sec != 0 || sendStamp.tv_usec != 0) && getTimeDiff(sendStamp, now) > QUERY_TIMEOUT)
		// {
		// 	std::cout << "error 411 on client " << it->second.client.getFd() << ": response timeout" << std::endl;
		// 	closeClient(it->second.client.getFd());
		// }
	}
}
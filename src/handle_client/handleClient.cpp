#include "Webserv.hpp"

void	Webserv::_newClient(int listenFd)
{
	struct sockaddr	clientAddr;
	socklen_t		addrSize = sizeof(clientAddr);
	std::time_t		now;

	int	clientFd = accept(listenFd, &clientAddr, &addrSize);
	if (clientFd == -1)
	{
		std::cout << "client not accepted" << std::endl;
		return;
	}

	int flags = fcntl(clientFd, F_GETFL, 0);
	if (flags == -1 || fcntl(clientFd, F_SETFL, flags | O_NONBLOCK) == -1)
	{
		close(clientFd);
		std::cerr << "fcntl fail" << std::endl;
		return ;
	}

	_clientMap[clientFd].client = Client(clientFd);
	_clientMap[clientFd].server = _serverMap[listenFd];
	_clientMap[clientFd].client.setFirstCoTimestamp(time(&now));

	struct epoll_event event;
	event.events = EPOLLIN;
	event.data.fd = clientFd;
	if (epoll_ctl(_epollFd, EPOLL_CTL_ADD, clientFd, &event) == -1)
		_closeClient(clientFd);
	time(&now);
	_clientMap[clientFd].client.getRequest().setRecvTimestamp(now);
	// std::cout << "Client (" << clientFd << ") connected" << std::endl;
}


void	Webserv::_handleResponse(int clientFd)
{
	Client&		    client = _clientMap[clientFd].client;
	const char*		ptr = client.getResponse().getRawResponse().c_str() + client.getBytesSent();
	size_t			remaining = client.getResponse().getToRead() - (client.getBytesSent());
	std::time_t  now;

	std::cout << "Sending" << std::string(ptr) <<  " to client (" << clientFd << ")" << std::endl;
	ssize_t bytesSentNow = send(clientFd, ptr, remaining, 0);
	time(&now);
	client.getResponse().setSendTimestamp(now);

	if (bytesSentNow > 0)
	{
		client.addBytesSent(bytesSentNow);
		if ((client.getBytesSent()) == client.getResponse().getToRead())
			_closeClient(clientFd);
	}
	else if (client.getCgiResponseState() != 1)
		_closeClient(clientFd);
}


void	Webserv::_closeClient(int clientFd)
{
	epoll_ctl(_epollFd, EPOLL_CTL_DEL, clientFd, NULL);
	_clientMap.erase(clientFd);
	close(clientFd);
	// std::cout << "Client (" << clientFd << ") disconnected" std::endl;
}

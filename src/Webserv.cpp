#include "../include/Webserv.hpp"

std::string	resolvePath(std::string relPath);

/*******************************************************************************
*						CTOR/DTOR
*******************************************************************************/


Webserv::Webserv(char **envp)
{
	for (int i = 0; envp[i] != NULL; i++)
	{
		this->_parentEnv.push_back(std::string(envp[i]));
	}
}

Webserv::Webserv(Webserv const &src):
	_servers(src._servers),
	_serverMap(src._serverMap),
	_clientMap(src._clientMap),
	_epollFd(src._epollFd),
	_parentEnv(src._parentEnv)
{}

Webserv	&Webserv::operator=(Webserv const &rhs)
{
	if (this != &rhs)
	{
		this->_servers = rhs._servers;
		this->_serverMap = rhs._serverMap;
		this->_clientMap = rhs._clientMap;
		this->_epollFd = rhs._epollFd; // close old epollFd ??
		this->_parentEnv = rhs._parentEnv;
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
		o << "\tcgi_max_output_size " << input.getServers()[i].getMaxCGIOutput() << ";" << std::endl;
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
			o << "\t\tcgi_max_output_size " << input.getServers()[i].getLocations()[j].getMaxCGIOutput() << ";" << std::endl;
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


void	Webserv::activityNotif(struct epoll_event	readyEvent)
{
	if (isListenSocket(readyEvent.data.fd))
		std::cout << "ListenSocket (" << readyEvent.data.fd << ") is ready for ";
	else if (isCgiFd(readyEvent.data.fd) != -1)
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


void	Webserv::launchServer(void)
{
	int					readyFds;
	struct epoll_event	readyEvents[200];
	long				idx = 0;

	this->_epollFd = this->setupEpoll();
	while (1)
	{
		// std::cout << "\nWaiting for connections" << std::endl;
		readyFds = epoll_wait(_epollFd, readyEvents, 200, -1);
		for (int i = 0; i < readyFds; i++)
		{
			activityNotif(readyEvents[i]);

			//error handling: throws only on epoll fail--> end program in main
			if (isListenSocket(readyEvents[i].data.fd))
				newClient(readyEvents[i].data.fd);
			else if ((idx = isCgiFd(readyEvents[i].data.fd)) != -1)
			{
				int servIdx = idx >> 16;
				int cgiIdx = idx & std::numeric_limits<int>::max();
				//both no throw
				if (readyEvents[i].events & EPOLLOUT)
					_handleCgiInput(this->_servers[servIdx].getCgiVec()[cgiIdx], this->_servers[servIdx]); //fd is CGI.writeFD
				else
					_handleCgiOutput(this->_servers[servIdx].getCgiVec()[cgiIdx], this->_servers[servIdx]); //fd is CGI.readFD
			}
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
	//TODO: clean client closing and exit when epoll timeouts
}


/*******************************************************************************
*						CLIENT HELPERS
*******************************************************************************/

//TODO: all exceptions here are not caught
void	Webserv::newClient(int listenFd)
{
	struct sockaddr	clientAddr;
	socklen_t		addrSize = sizeof(clientAddr);
	struct timeval	now;

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

	struct epoll_event event;
	event.events = EPOLLIN;
	event.data.fd = clientFd;
	if (epoll_ctl(_epollFd, EPOLL_CTL_ADD, clientFd, &event) == -1)
	{
		_clientMap.erase(clientFd);
		close(clientFd); //TODO: refine throwing logic on epoll error
		throw(std::runtime_error(std::strerror(errno)));
	}
	gettimeofday(&now, NULL);
	_clientMap[clientFd].client.getRequest().setRecvTimestamp(now);
	std::cout << "New client with fd: " << clientFd << std::endl;
}


void	Webserv::testPrint(int clientFd, Client &client)
{
	std::cout << client.getRequest() << std::endl;
	std::cout << "~~~~~~ end request ~~~~~~~" << std::endl;
	(void)client;
	_clientMap[clientFd].client.clearReadBuffer();
}


void	Webserv::handleRequest(int clientFd)
{
	Client&		    client = _clientMap[clientFd].client;
	char		    buffer[4056];
	struct timeval	now;

	ssize_t bytesRead = recv(clientFd, buffer, sizeof(buffer) - 1, 0);
	//logic for timeout handling
	gettimeofday(&now, NULL);
	client.getRequest().setRecvTimestamp(now);
	int	lexReturn = -2;

 	if (bytesRead < 1)
		return (closeClient(clientFd));

	buffer[bytesRead] = '\0';
	// std::cout << "Recv from client (" << clientFd << ") : " << std::string(buffer);
	client.appendReadBuffer(std::string(buffer, bytesRead));//data string
	if (!client.getRequest().getHeaderFlag())
	{	
		try
		{
             std::cout << client.getReadBuffer() << std::endl;
			lexReturn = client.getRequest().lexRawData(client.getReadBuffer());
		}
		catch(const Request::Error405 &e)
		{
			client.getResponse().build405Response(true, true, true, this->_clientMap[clientFd].server, NULL);
			lexReturn = -1; //to get into write response logic
		}
		catch(const Request::ErrorNum &e)
		{
			client.getResponse().buildErrorResponse(e.getCode(), this->_clientMap[clientFd].server, NULL);
			lexReturn = -1; //to get into write response logic
		}
		catch(const std::exception& e)
		{
			std::cerr << e.what() << '\n';
			client.getResponse().buildErrorResponse(500, this->_clientMap[clientFd].server, NULL);
			lexReturn = -1; //to get into write response logic
		}
	}
	else
	{
		int tempFD = open(client.getRequest().getBodyFilename().c_str(), O_WRONLY | O_APPEND);
		write(tempFD, client.getReadBuffer().c_str(), client.getReadBuffer().size());
		close(tempFD);
		client.getRequest().addBytesRead(client.getReadBuffer().size());
		client.getReadBuffer().clear();
		if (client.getRequest().getContentLength() - client.getRequest().getBytesRead() <= 0)
		{
			lexReturn = -1;
			client.getRequest().setReqFlag(true);
		}
	}
	if (lexReturn == -1)
	{
		testPrint(clientFd, client); //put request dipsatcher here, build body here
		this->_clientMap[clientFd].server->dispatchRequest(client, this->_epollFd); //CGI added to epoll ctl in this function

		if (this->_clientMap[clientFd].client.getCgiResponseState() == 0)
		{
			struct epoll_event event;
			event.events = EPOLLOUT;
			event.data.fd = clientFd;
			if (epoll_ctl(_epollFd, EPOLL_CTL_MOD, clientFd, &event) == -1)
			{
				_clientMap.erase(clientFd);
				close(clientFd);
				throw(std::runtime_error(std::strerror(errno))); //exception is not being caught
			}
		}
		else //remove clientFD from epoll while we are reading the content from the CGI
		{
			struct epoll_event event;
			if (epoll_ctl(_epollFd, EPOLL_CTL_DEL, clientFd, &event) == -1)
			{
				_clientMap.erase(clientFd);
				close(clientFd);
				throw(std::runtime_error(std::strerror(errno))); //exception is not being caught
			}
		}
	}
}

void    Webserv::_destroyCGI(int fd, Server &server)//readFD or writeFD of CGI
{
	epoll_ctl(this->_epollFd, EPOLL_CTL_DEL, fd, NULL);
	close(fd);
	long cgiID = isCgiFd(fd) & std::numeric_limits<int>::max();
	if (fd == server.getCgiVec()[cgiID].getWriteFD())
		close(server.getCgiVec()[cgiID].getReadFD());
	int	status;
	waitpid(server.getCgiVec()[cgiID].getPID(), &status, WNOHANG); //non blocking 
	if (WIFEXITED(status) && WEXITSTATUS(status) != 0)
	{
		// script error,  return appropriate code
	}
	std::vector<CGI>::iterator remove(server.getCgiVec().begin() + (cgiID));
	server.getCgiVec().erase(remove);
	return ;
}

//is buffeer placeholder for where we want to write actually ?
void	Webserv::_handleCgiInput(CGI &cgi, Server &server)
{
	Client      &client = this->_clientMap[cgi.getClientFD()].client;
	long long   contentLength = client.getRequest().getContentLength();
	char	    buffer[4056];
	
	//TODO: do we need this ?
	//logic to change as we're either reading directly
	// int     fileFD = open(client.getRequest().getBodyFilename().c_str(), O_RDONLY);
	// if (fileFD == -1)
	// {
	// 	struct epoll_event	event;
	// 	event.events = EPOLLOUT;
	// 	event.data.fd = cgi.getClientFD();
	// 	if (epoll_ctl(this->_epollFd, EPOLL_CTL_ADD, cgi.getClientFD(), &event) == -1)
	// 		closeClient(cgi.getClientFD());
	// 	this->_clientMap[cgi.getClientFD()].client.setCgiResponseState(2);
	// 	this->_clientMap[cgi.getClientFD()].client.getResponse().buildErrorResponse(502, &server, &cgi.getLocation());
	// 	_destroyCGI(cgi.getWriteFD(), server);
	// }
	ssize_t bytesRead = read(cgi.getInFileFD(), buffer, sizeof(buffer) - 1);

	if (bytesRead > 0)
	{
		buffer[bytesRead] = 0;
		ssize_t bytesSentNow = write(cgi.getWriteFD(), buffer, bytesRead);
		cgi.addBytesWritten(bytesSentNow);
		if (cgi.getBytesWritten() >= contentLength)
		{
			epoll_ctl(this->_epollFd, EPOLL_CTL_DEL, cgi.getWriteFD(), NULL);
			close(cgi.getWriteFD());
			close(cgi.getInFileFD());
		}
	}
	else
	{
		close(cgi.getInFileFD());
		this->_clientMap[cgi.getClientFD()].client.setCgiResponseState(2);
		this->_clientMap[cgi.getClientFD()].client.getResponse().buildErrorResponse(502, &server, &cgi.getLocation());
		_destroyCGI(cgi.getWriteFD(), server);
		struct epoll_event	event;
		event.events = EPOLLOUT;
		event.data.fd = cgi.getClientFD();
		if (epoll_ctl(this->_epollFd, EPOLL_CTL_ADD, cgi.getClientFD(), &event) == -1)
			closeClient(cgi.getClientFD());
	}
}

void	Webserv::_cgiError(CGI &cgi)
{
	if (cgi.getContentLength() == -1)
	{
		struct epoll_event	event;
		this->_clientMap[cgi.getClientFD()].client.setCgiResponseState(2); //cgi response done
		this->_clientMap[cgi.getClientFD()].client.getResponse().buildErrorResponse(502, this->_clientMap[cgi.getClientFD()].server, &cgi.getLocation());
		event.events = EPOLLOUT;
		event.data.fd = cgi.getClientFD();
		if (epoll_ctl(this->_epollFd, EPOLL_CTL_ADD, cgi.getClientFD(), &event) == -1)
		{
			closeClient(cgi.getClientFD());
		}
	}
	else
	{
		closeClient(cgi.getClientFD());
	}
	_destroyCGI(cgi.getReadFD(), *(this->_clientMap[cgi.getClientFD()].server));
}

int	Webserv::_setupCGIResponseHeaders(CGI &cgi, long long maxOutSize)
{
	int	lexReturn;

	try
	{
		lexReturn = cgi.lexCGIOutput(cgi.getOutBuff());
	}
	catch(const std::exception& e)
	{
		std::cout << e.what() << std::endl;
		return (-2);
	}
	if (lexReturn != 0 && this->_clientMap[cgi.getClientFD()].client.getResponse().getContentType().empty())//first time done header parsing 
	{
		if (cgi.getContentType().empty() || (maxOutSize != -1 && cgi.getContentLength() > maxOutSize) || (maxOutSize == -1 && cgi.getContentLength() == -1) || cgi.getStatus() == -1)
		{
			return (-2);
		}
		this->_clientMap[cgi.getClientFD()].client.getResponse().setContentType(cgi.getContentType());
		if (cgi.getStatus() != -1)
			this->_clientMap[cgi.getClientFD()].client.getResponse().setReturnCode(cgi.getStatus());
			// stop cgi if status 4xx or 5xx and build errror response
        if (!cgi.getSetCookie().empty()) //set cookie
            this->_clientMap[cgi.getClientFD()].client.getResponse().setSetCookie(cgi.getSetCookie());
		if (cgi.getContentLength() != -1)
		{
			std::stringstream	stream;
			struct epoll_event	event;

			stream << cgi.getContentLength();
			this->_clientMap[cgi.getClientFD()].client.getResponse().setContentLength(stream.str());
			this->_clientMap[cgi.getClientFD()].client.getResponse().setToRead(cgi.getContentLength());
			this->_clientMap[cgi.getClientFD()].client.getResponse().buildRawResponse();
			event.events = EPOLLOUT;
			event.data.fd = cgi.getClientFD();
			if (epoll_ctl(this->_epollFd, EPOLL_CTL_ADD, cgi.getClientFD(), &event) == -1)
			{
				closeClient(cgi.getClientFD());
				_destroyCGI(cgi.getReadFD(), *(this->_clientMap[cgi.getClientFD()].server));
				return (-3);
			}
		}
	}
	return (lexReturn);
}

void	Webserv::_handleCgiOutput(CGI &cgi, Server &server)
{
	char			buffer[4056];
	int				lexReturn = -4;
	struct timeval	now;

	ssize_t	bytesRead = read(cgi.getReadFD(), buffer, sizeof(buffer) - 1);
	//logic for timeout handling
	gettimeofday(&now, NULL);
	cgi.setOutTimestamp(now);
	if (bytesRead != -1)
	{
		long long	maxOutSize = cgi.getLocation().getMaxCGIOutput();
		if (maxOutSize == -1)
			maxOutSize = server.getMaxCGIOutput();
		buffer[bytesRead] = '\0';
		cgi.getOutBuff().append(buffer);
		if (this->_clientMap[cgi.getClientFD()].client.getResponse().getContentType().empty())
		{
			if ((lexReturn = _setupCGIResponseHeaders(cgi, maxOutSize)) == -2)
				return(cgi.setCGIContentLength(-1), _cgiError(cgi));
			else if (lexReturn == -3)
				return ;
		}
		if (lexReturn != 0)
		{
			if (cgi.getContentLength() != -1)
			{
				this->_clientMap[cgi.getClientFD()].client.getResponse().getRawResponse().append(cgi.getOutBuff());
				cgi.addBytesSent(cgi.getOutBuff().length());
				cgi.getOutBuff().clear();
				if (cgi.getBytesSent() >= cgi.getContentLength())
				{
					struct epoll_event	event;
					epoll_ctl(this->_epollFd, EPOLL_CTL_DEL, cgi.getReadFD(), &event);
					this->_clientMap[cgi.getClientFD()].client.setCgiResponseState(2); //cgi response done
					_destroyCGI(cgi.getReadFD(), *(this->_clientMap[cgi.getClientFD()].server));
				}
			}
			else
			{
				if (bytesRead == 0) //we are done reading
				{
					struct epoll_event	event;
					std::stringstream	stream;

					epoll_ctl(this->_epollFd, EPOLL_CTL_DEL, cgi.getReadFD(), &event);
					stream << cgi.getBytesSent();
					this->_clientMap[cgi.getClientFD()].client.getResponse().setContentLength(stream.str());
					this->_clientMap[cgi.getClientFD()].client.getResponse().setToRead(cgi.getBytesSent());
					this->_clientMap[cgi.getClientFD()].client.getResponse().buildRawResponse();
					this->_clientMap[cgi.getClientFD()].client.setCgiResponseState(2); //cgi response done
					event.events = EPOLLOUT;
					event.data.fd = cgi.getClientFD();
					if (epoll_ctl(this->_epollFd, EPOLL_CTL_ADD, cgi.getClientFD(), &event) == -1)
						closeClient(cgi.getClientFD());
					_destroyCGI(cgi.getReadFD(), *(this->_clientMap[cgi.getClientFD()].server));
					return ;
				}
				this->_clientMap[cgi.getClientFD()].client.getResponse().getEntityBody().append(cgi.getOutBuff());
				cgi.addBytesSent(cgi.getOutBuff().length());
				cgi.getOutBuff().clear();
				if (cgi.getBytesSent() > maxOutSize)
					return (_cgiError(cgi));
			}
		}
	}
	else
		return (_cgiError(cgi));
}

void	Webserv::handleResponse(int clientFd)
{
	Client&		    client = _clientMap[clientFd].client;
	const char*		ptr = client.getResponse().getRawResponse().c_str() + client.getBytesSent();
	size_t			remaining = client.getResponse().getToRead() - (client.getBytesSent());
	struct timeval  now;

	// std::cout << "Sending" << std::string(ptr) <<  " to client (" << clientFd << ")" << std::endl;
	ssize_t bytesSentNow = send(clientFd, ptr, remaining, 0);
	gettimeofday(&now, NULL); //(stdtime could work cuz timeout ~ 30-60s)
	client.getResponse().setSendTimestamp(now);

	if (bytesSentNow > 0)
	{
		client.addBytesSent(bytesSentNow);
		if ((client.getBytesSent()) == client.getResponse().getToRead())
			closeClient(clientFd);
	}
	else if (client.getCgiResponseState() != 1)
		closeClient(clientFd);
}


void	Webserv::closeClient(int clientFd)
{
	epoll_ctl(_epollFd, EPOLL_CTL_DEL, clientFd, NULL);
	_clientMap.erase(clientFd);
	close(clientFd);
	std::cout << "Client disconnected with fd: " << clientFd << std::endl;
}


/*******************************************************************************
*						BOOL FD IS
*******************************************************************************/

bool	Webserv::isListenSocket(int fd) const
{
	if (_serverMap.find(fd) != _serverMap.end())
		return true;
	return false;
}


long  Webserv::isCgiFd(int fd) //return a pair to identify server and CGI OR a bitshifted number ??? like return codes and error codes
{
	for (size_t j = 0; j != this->_servers.size(); j++)
	{
		for (size_t i = 0; i != this->_servers[j].getCgiVec().size(); i++)
		{
			if (fd == this->_servers[j].getCgiVec()[i].getReadFD() || fd == this->_servers[j].getCgiVec()[i].getWriteFD())
				return ((static_cast<int>(j) << 16) | static_cast<int>(i));
		}
	}
	return (-1);
}
//t2 - t1
int	getTimeDiff(timeval t1, timeval t2)
{
	long dSeconds = t2.tv_sec  - t1.tv_sec;
	long dUseconds = t2.tv_usec - t1.tv_usec;

	return ((dSeconds) * 1000 + dUseconds / 1000.0) + 0.5; //+0.5 is a rounding technique to be sure we round the nearest integer.
}

//TODO: implement a timeout if CGI is stuck in infinite loop. Just .erase() as waitpid(WNOHANG) in CGI destructor
void    Webserv::handleTimeouts(void)
{
	struct timeval	now;
	struct timeval	recvStamp;
	struct timeval	sendStamp;
	bool			reqFlag;
	bool			responseFlag;
	struct timeval	cgiOutStamp;
	
	gettimeofday(&now, NULL);
	for (std::map<int, t_connection>::iterator it = this->_clientMap.begin(); it != this->_clientMap.end(); it++)
	{
		Client	&client = it->second.client;
		recvStamp = client.getRequest().getRecvTimestamp();
		sendStamp = client.getResponse().getSendTimestamp();
		reqFlag = client.getRequest().getReqFlag();
		responseFlag = client.getResponse().getRespFlag();

		//check if 1) request/response complete 2) timestamp initialized === first receive/send happend 3) timeout status
		if (!reqFlag && (recvStamp.tv_sec != 0 || recvStamp.tv_usec != 0) && getTimeDiff(recvStamp, now) > QUERY_TIMEOUT)
		{
			client.getResponse().buildErrorResponse(408, it->second.server, NULL);
			closeClient(client.getFd());
		}
		if (!responseFlag && (sendStamp.tv_sec != 0 || sendStamp.tv_usec != 0) && getTimeDiff(sendStamp, now) > QUERY_TIMEOUT)
		{
			client.getResponse().buildErrorResponse(408, it->second.server, NULL);
			closeClient(client.getFd());
		}
	}
	for (size_t j = 0; j != this->_servers.size(); j++)
	{
		for (size_t i = 0; i != this->_servers[j].getCgiVec().size(); i++)
		{
			CGI	&cgi = this->_servers[j].getCgiVec()[i];
			cgiOutStamp = cgi.getOutTimestamp();
			if (this->_clientMap[cgi.getClientFD()].client.getCgiResponseState() == 1 && (cgiOutStamp.tv_sec != 0 || cgiOutStamp.tv_usec != 0) && getTimeDiff(cgiOutStamp, now) > QUERY_TIMEOUT)
			{
				this->_clientMap[cgi.getClientFD()].client.getResponse().buildErrorResponse(503, &this->_servers[j], &cgi.getLocation());
				// TODO erase cgi from cgi map
			}
		}
	}
}

#include "Webserv.hpp"

void    Webserv::_destroyCGI(CGI &cgi, Server &server)//readFD or writeFD of CGI
{
	if (cgi.getWriteFD() != -1)
	{
		close(cgi.getWriteFD());
		cgi.getWriteFD() = -1;
	}
	if (cgi.getReadFD() != -1)
	{
		close(cgi.getReadFD());
		cgi.getReadFD() = -1;
	}
	if (cgi.getInFileFD() != -1)
	{
		close(cgi.getInFileFD());
		cgi.getInFileFD() = -1;
	}
	//check waitpid return and kill process if still running
	int state = waitpid(cgi.getPID(), NULL, WNOHANG); //non blocking with WNOHANG
	if (state == 0)
		kill(cgi.getPID(), SIGKILL);
	size_t	i;
	for (i = 0; i < server.getCgiVec().size(); i++)
	{
		if (&(server.getCgiVec()[i]) == &cgi)
			break;
	}
	std::vector<CGI>::iterator	remove(server.getCgiVec().begin() + i);
	server.getCgiVec().erase(remove);
	return ;
}

//is buffeer placeholder for where we want to write actually ?
void	Webserv::_handleCgiInput(CGI &cgi, Server &server)
{
	Client      &client = this->_clientMap[cgi.getClientFD()].client;
	long long   contentLength = client.getRequest().getContentLength();
	char	    buffer[4056];

	ssize_t bytesRead = read(cgi.getInFileFD(), buffer, sizeof(buffer) - 1);

	if (bytesRead > 0)
	{

		buffer[bytesRead] = 0;
		errno = 0;
		ssize_t bytesSentNow = write(cgi.getWriteFD(), buffer, bytesRead);
		std::cout << "cgi input buffer: " << buffer << std::endl;
		std::cout << "bytessentnow: " << bytesSentNow << std::endl;
		cgi.addBytesWritten(bytesSentNow);
		if (cgi.getBytesWritten() >= contentLength)
		{
			epoll_ctl(this->_epollFd, EPOLL_CTL_DEL, cgi.getWriteFD(), NULL);
			close(cgi.getWriteFD());
			cgi.getWriteFD() = -1;
			close(cgi.getInFileFD());
			cgi.getInFileFD() = -1;
		}
	}
	else
	{
		std::cout << "in error reading from file cgi input" << std::endl;
		close(cgi.getInFileFD());
		cgi.getInFileFD() = -1;
		this->_clientMap[cgi.getClientFD()].client.setCgiResponseState(2);
		this->_clientMap[cgi.getClientFD()].client.getResponse().buildErrorResponse(500, &server, &cgi.getLocation());
		epoll_ctl(this->_epollFd, EPOLL_CTL_DEL, cgi.getWriteFD(), NULL);
		epoll_ctl(this->_epollFd, EPOLL_CTL_DEL, cgi.getReadFD(), NULL);
		_destroyCGI(cgi, server);
		struct epoll_event	event;
		event.events = EPOLLOUT;
		event.data.fd = cgi.getClientFD();
		if (epoll_ctl(this->_epollFd, EPOLL_CTL_ADD, cgi.getClientFD(), &event) == -1)
			_closeClient(cgi.getClientFD());
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
			_closeClient(cgi.getClientFD());
		}
	}
	else
	{
		_closeClient(cgi.getClientFD());
	}
	epoll_ctl(this->_epollFd, EPOLL_CTL_DEL, cgi.getReadFD(), NULL);
	_destroyCGI(cgi, *(this->_clientMap[cgi.getClientFD()].server));
}

void	Webserv::_buildOtherCode(CGI &cgi)
{
	struct epoll_event	event;
	Server				&server = *this->_clientMap[cgi.getClientFD()].server;

	epoll_ctl(this->_epollFd, EPOLL_CTL_DEL, cgi.getReadFD(), &event);
	if (cgi.getStatus() == 300 || cgi.getStatus() == 301 || cgi.getStatus() == 302)
	{
		if (cgi.getLocationHeader().empty())
			this->_clientMap[cgi.getClientFD()].client.getResponse().buildErrorResponse(502, &server, &cgi.getLocation());
		else
			this->_clientMap[cgi.getClientFD()].client.getResponse().buildRedirResponse(cgi.getStatus(), cgi.getLocationHeader());
	}
	else if (cgi.getStatus() == 405)
		this->_clientMap[cgi.getClientFD()].client.getResponse().build405Response(server.isMethodAllowed(GET, cgi.getLocation()), server.isMethodAllowed(POST, cgi.getLocation()), server.isMethodAllowed(DELETE, cgi.getLocation()), &server, &cgi.getLocation());
	else
		this->_clientMap[cgi.getClientFD()].client.getResponse().buildErrorResponse(cgi.getStatus(), &server, &cgi.getLocation());
	event.events = EPOLLOUT;
	event.data.fd = cgi.getClientFD();
	if (epoll_ctl(this->_epollFd, EPOLL_CTL_ADD, cgi.getClientFD(), &event) == -1)
		_closeClient(cgi.getClientFD());
	_destroyCGI(cgi, *(this->_clientMap[cgi.getClientFD()].server));
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
	if (lexReturn != 0 && this->_clientMap[cgi.getClientFD()].client.getResponse().getReturnCode() == 0)//first time done header parsing 
	{
		if (cgi.getStatus() == -1)
			return (-2);
		this->_clientMap[cgi.getClientFD()].client.getResponse().setReturnCode(cgi.getStatus());
		if (cgi.getStatus() > 299)
			return (_buildOtherCode(cgi), -3);
		if (cgi.getContentType().empty() || (maxOutSize != -1 && cgi.getContentLength() > maxOutSize) || (maxOutSize == -1 && cgi.getContentLength() == -1) || cgi.getStatus() == -1)
			return (-2);
		this->_clientMap[cgi.getClientFD()].client.getResponse().setContentType(cgi.getContentType());
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
				_closeClient(cgi.getClientFD());
				epoll_ctl(this->_epollFd, EPOLL_CTL_DEL, cgi.getReadFD(), NULL);
				_destroyCGI(cgi, *(this->_clientMap[cgi.getClientFD()].server));
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

	std::cout << "in CGI output" << std::endl;
	ssize_t	bytesRead = read(cgi.getReadFD(), buffer, sizeof(buffer) - 1);
	std::cout << "bytesread: " << bytesRead << std::endl;
	if (bytesRead == -1)
		return(_cgiError(cgi));

	long long	maxOutSize = cgi.getLocation().getMaxCGIOutput();
	if (maxOutSize == -1)
		maxOutSize = server.getMaxCGIOutput();
	buffer[bytesRead] = '\0';
	cgi.getOutBuff().append(buffer);
	if (this->_clientMap[cgi.getClientFD()].client.getResponse().getReturnCode() == 0)
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
				epoll_ctl(this->_epollFd, EPOLL_CTL_DEL, cgi.getReadFD(), NULL);
				_destroyCGI(cgi, *(this->_clientMap[cgi.getClientFD()].server));
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
					_closeClient(cgi.getClientFD());
				epoll_ctl(this->_epollFd, EPOLL_CTL_DEL, cgi.getReadFD(), NULL);
				_destroyCGI(cgi, *(this->_clientMap[cgi.getClientFD()].server));
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

void	Webserv::_handleErrorPipe(CGI &cgi)
{
	int					err;
	struct epoll_event	event;
	size_t				n = read(cgi.getErrorFD(), &err, sizeof(err));
	Client				&client = this->_clientMap[cgi.getClientFD()].client;

	if (n > 0)
	{
		//exceve error, cleanup time and build error response
		epoll_ctl(this->_epollFd, EPOLL_CTL_DEL, cgi.getErrorFD(), &event);
		_destroyCGI(cgi, *(this->_clientMap[cgi.getClientFD()].server));
		event.events = EPOLLOUT;
		event.data.fd = cgi.getClientFD();
		if (epoll_ctl(_epollFd, EPOLL_CTL_ADD, cgi.getClientFD(), &event) == -1)
		{
			_closeClient(cgi.getClientFD());
			return;
		}
		return (client.getResponse().buildErrorResponse(500, this->_clientMap[cgi.getClientFD()].server, &cgi.getLocation()));
	}
	//add readfd/writefd to epoll and close errorPipe
	epoll_ctl(this->_epollFd, EPOLL_CTL_DEL, cgi.getErrorFD(), &event);
	if (cgi.getWriteFD() != -1)
	{
		event.events = EPOLLOUT;
		event.data.fd = cgi.getWriteFD();
		if (epoll_ctl(_epollFd, EPOLL_CTL_ADD, cgi.getWriteFD(), &event) == -1)
		{
			event.events = EPOLLOUT;
			event.data.fd = cgi.getClientFD();
			if (epoll_ctl(_epollFd, EPOLL_CTL_ADD, cgi.getClientFD(), &event) == -1)
			{
				_closeClient(cgi.getClientFD());
				return;
			}
			return (client.getResponse().buildErrorResponse(500, this->_clientMap[cgi.getClientFD()].server, &cgi.getLocation()));
		}
	}
	if (cgi.getReadFD() != -1)
	{
		event.events = EPOLLIN;
		event.data.fd = cgi.getReadFD();
		if (epoll_ctl(_epollFd, EPOLL_CTL_ADD, cgi.getReadFD(), &event) == -1)
		{
			if (cgi.getWriteFD() != -1)
				epoll_ctl(this->_epollFd, EPOLL_CTL_DEL, cgi.getWriteFD(), &event);
			event.events = EPOLLOUT;
			event.data.fd = cgi.getClientFD();
			if (epoll_ctl(_epollFd, EPOLL_CTL_ADD, cgi.getClientFD(), &event) == -1)
			{
				_closeClient(cgi.getClientFD());
				return;
			}
			return (client.getResponse().buildErrorResponse(500, this->_clientMap[cgi.getClientFD()].server, &cgi.getLocation()));
		}
	}
	close(cgi.getErrorFD());
	cgi.getErrorFD() = -1;
	return ;
}
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

void	Webserv::_cgiInputError(Server& server, CGI& cgi, const std::string& message)
{
	std::cout << message << std::endl;
	close(cgi.getInFileFD());
	cgi.getInFileFD() = -1;
	this->_clientMap[cgi.getClientFD()].client.setCgiResponseState(2);
	this->_clientMap[cgi.getClientFD()].client.getResponse().buildErrorResponse(500, &server, &cgi.getLocation());
	epoll_ctl(this->_epollFd, EPOLL_CTL_DEL, cgi.getWriteFD(), NULL);
	epoll_ctl(this->_epollFd, EPOLL_CTL_DEL, cgi.getReadFD(), NULL);
	_destroyCGI(cgi, server);
	_modifyEpoll(EPOLLOUT, EPOLL_CTL_ADD, cgi.getClientFD());	
}

//is buffeer placeholder for where we want to write actually ?
void	Webserv::_handleCgiInput(CGI &cgi, Server &server)
{
	Client      &client = this->_clientMap[cgi.getClientFD()].client;
	long long   contentLength = client.getRequest().getContentLength();
	char	    buffer[4056];

	ssize_t bytesRead = read(cgi.getInFileFD(), buffer, sizeof(buffer) - 1);

	if (bytesRead < 1)
		 return (_cgiInputError(server, cgi, "in error reading from file cgi input"));
	
	buffer[bytesRead] = 0;
	ssize_t bytesSentNow = write(cgi.getWriteFD(), buffer, bytesRead);
	if (bytesSentNow < 1)
		return (_cgiInputError(server, cgi, "in error writing to file cgi input"));

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

void	Webserv::_cgiError(CGI &cgi)
{
	if (cgi.getContentLength() == -1)
	{
		this->_clientMap[cgi.getClientFD()].client.setCgiResponseState(2); //cgi response done
		this->_clientMap[cgi.getClientFD()].client.getResponse().buildErrorResponse(502, this->_clientMap[cgi.getClientFD()].server, &cgi.getLocation());
		_modifyEpoll(EPOLLOUT, EPOLL_CTL_ADD, cgi.getClientFD());
	}
	else
		_closeClient(cgi.getClientFD());
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
	_modifyEpoll(EPOLLOUT, EPOLL_CTL_ADD, cgi.getClientFD());
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
		return (_returnError500(cgi, client));
	}
	//add readfd/writefd to epoll and close errorPipe
	epoll_ctl(this->_epollFd, EPOLL_CTL_DEL, cgi.getErrorFD(), &event);
	if (cgi.getWriteFD() != -1)
	{
		event.events = EPOLLOUT;
		event.data.fd = cgi.getWriteFD();
		if (epoll_ctl(_epollFd, EPOLL_CTL_ADD, cgi.getWriteFD(), &event) == -1)
			return (_returnError500(cgi, client));
	}
	if (cgi.getReadFD() != -1)
	{
		event.events = EPOLLIN;
		event.data.fd = cgi.getReadFD();
		if (epoll_ctl(_epollFd, EPOLL_CTL_ADD, cgi.getReadFD(), &event) == -1)
		{
			if (cgi.getWriteFD() != -1)
				epoll_ctl(this->_epollFd, EPOLL_CTL_DEL, cgi.getWriteFD(), &event);
			return (_returnError500(cgi, client));
		}
	}
	close(cgi.getErrorFD());
	cgi.getErrorFD() = -1;
	return ;
}

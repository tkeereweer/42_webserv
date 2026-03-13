#include "Webserv.hpp"

void	Webserv::_handleCgiOutput(CGI &cgi, Server &server)
{
	char			buffer[4056];
	int				lexReturn = -4;

	ssize_t	bytesRead = read(cgi.getReadFD(), buffer, sizeof(buffer) - 1);
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
	if (lexReturn == 0)
		return ;
	if (cgi.getContentLength() != -1)
	{
		this->_clientMap[cgi.getClientFD()].client.getResponse().getRawResponse().append(cgi.getOutBuff());
		cgi.addBytesSent(cgi.getOutBuff().length());
		cgi.getOutBuff().clear();
		if (cgi.getBytesSent() >= cgi.getContentLength())
            _setupInvalidCgi(cgi);
	}
	else
	{
		if (bytesRead == 0) //we are done reading
			return (_returnValidCgi(cgi));
		this->_clientMap[cgi.getClientFD()].client.getResponse().getEntityBody().append(cgi.getOutBuff());
		cgi.addBytesSent(cgi.getOutBuff().length());
		cgi.getOutBuff().clear();
		if (cgi.getBytesSent() > maxOutSize)
			return (_cgiError(cgi));
	}
}

void	Webserv::_setupInvalidCgi(CGI &cgi)
{
	struct epoll_event	event;
	epoll_ctl(this->_epollFd, EPOLL_CTL_DEL, cgi.getReadFD(), &event);
	this->_clientMap[cgi.getClientFD()].client.setCgiResponseState(2); //cgi response done
	epoll_ctl(this->_epollFd, EPOLL_CTL_DEL, cgi.getReadFD(), NULL);
	_destroyCGI(cgi, *(this->_clientMap[cgi.getClientFD()].server));
}

void	Webserv::_returnValidCgi(CGI &cgi)
{
	struct epoll_event	event;
	std::stringstream	stream;

	epoll_ctl(this->_epollFd, EPOLL_CTL_DEL, cgi.getReadFD(), &event);
	stream << cgi.getBytesSent();
	this->_clientMap[cgi.getClientFD()].client.getResponse().setContentLength(stream.str());
	this->_clientMap[cgi.getClientFD()].client.getResponse().setToRead(cgi.getBytesSent());
	this->_clientMap[cgi.getClientFD()].client.getResponse().buildRawResponse();
	this->_clientMap[cgi.getClientFD()].client.setCgiResponseState(2); //cgi response done
	_modifyEpoll(EPOLLOUT, EPOLL_CTL_ADD, cgi.getClientFD());
	epoll_ctl(this->_epollFd, EPOLL_CTL_DEL, cgi.getReadFD(), NULL);
	_destroyCGI(cgi, *(this->_clientMap[cgi.getClientFD()].server));
	return ;	
}

void	Webserv::_returnError500(CGI &cgi, Client &client)
{
	struct epoll_event	event;
	event.events = EPOLLOUT;
	event.data.fd = cgi.getClientFD();
	if (epoll_ctl(this->_epollFd, EPOLL_CTL_ADD, cgi.getClientFD(), &event) == -1)
	{
		_closeClient(cgi.getClientFD());
		return;
	}
	return (client.getResponse().buildErrorResponse(500, this->_clientMap[cgi.getClientFD()].server, &cgi.getLocation()));
}
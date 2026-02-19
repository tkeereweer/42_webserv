#include "../include/Client.hpp"

/*******************************************************************************
*						CTOR/DTOR
*******************************************************************************/

Client::Client(void) : _bytesSent(0)
{}

Client::Client(int fd) : 
	_fd(fd),
	_bytesSent(0)
{}

Client::Client(Client const &src) :
	_fd(src._fd),
	_request(src._request),
	_response(src._response),
	_bytesSent(src._bytesSent)
{}

Client&	Client::operator=(Client const &rhs)
{
	if (this != &rhs)
	{
		_fd = rhs._fd;
		_request = rhs._request;
		_response = rhs._response;
		_bytesSent = rhs._bytesSent;
	}
	return (*this);
}

Client::~Client(void)
{}


/*******************************************************************************
*						GET/SET/CLEAR
*******************************************************************************/

int			Client::getFd(void) const
{
	return (_fd);
}

const std::string&	Client::getRequest(void) const
{
	return (_request);
}

const std::string&	Client::getResponse(void) const
{
	return (_response);
}

size_t		Client::getBytesSent(void) const
{
	return (_bytesSent);
}



void		Client::setRequest(const std::string& request)
{
	this->_request = request;
}

void		Client::setResponse(const std::string& response)
{
	this->_response = response;
}

void		Client::appendRequest(const std::string& appendix)
{
	_request += appendix;
}

void		Client::addBytesSent(size_t bytesSent)
{
	_bytesSent += bytesSent;
}


void		Client::clearRequest(void)
{
	_request.clear();
}

void		Client::clearResponse(void)
{
	_response.clear();
}

void		Client::clearBytesSent(void)
{
	_bytesSent = 0;
}

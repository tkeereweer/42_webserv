#include "../include/Client.hpp"

/*******************************************************************************
*						CTOR/DTOR
*******************************************************************************/

Client::Client(void)
{}

Client::Client(int fd) : _fd(fd)
{}

Client::Client(Client const &src) :
	_fd(src._fd),
	_request(src._request),
	_response(src._response)
{}

Client&	Client::operator=(Client const &rhs)
{
	if (this != &rhs)
	{
		_fd = rhs._fd;
		_request = rhs._request;
		_response = rhs._response;
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

std::string	Client::getRequest(void) const
{
	return (_request);
}

std::string	Client::getResponse(void) const
{
	return (_response);
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


void		Client::clearRequest(void)
{
	_request.clear();
}

void		Client::clearResponse(void)
{
	_response.clear();
}

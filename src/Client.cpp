#include "../include/Client.hpp"

/*******************************************************************************
*						CTOR/DTOR
*******************************************************************************/

Client::Client(void) : _request(), _bytesSent(0), _requestDone(false), _responseDone(false)
{}

Client::Client(int fd) : 
	_fd(fd),
    _request(),
	_bytesSent(0)
{}

Client::Client(Client const &src) :
	_fd(src._fd),
	_request(src._request),
	_response(src._response),
	_bytesSent(src._bytesSent),
    _requestDone(false),
    _responseDone(false)
    {}

Client&	Client::operator=(Client const &rhs)
{
	if (this != &rhs)
	{
		_fd = rhs._fd;
		_request = rhs._request;
		_response = rhs._response;
		_bytesSent = rhs._bytesSent;
        _requestDone = rhs._requestDone;
        _responseDone = rhs._responseDone;
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

std::string&	Client::getReadBuffer(void)
{
	return (_readBuffer);
}

const std::string&	Client::getResponse(void) const
{
	return (_response);
}

size_t		Client::getBytesSent(void) const
{
	return (_bytesSent);
}



void		Client::setReadBuffer(const std::string& readBuffer)
{
	this->_readBuffer = readBuffer;
}

void		Client::setResponse(const std::string& response)
{
	this->_response = response;
}

void		Client::appendReadBuffer(const std::string& appendix)
{
	_readBuffer += appendix;
}

void		Client::addBytesSent(size_t bytesSent)
{
	_bytesSent += bytesSent;
}


void		Client::clearReadBuffer(void)
{
	_readBuffer.clear();
}

void		Client::clearResponse(void)
{
	_response.clear();
}

void		Client::clearBytesSent(void)
{
	_bytesSent = 0;
}

Request &Client::getRequest(void)
{
    return (this->_request);
}

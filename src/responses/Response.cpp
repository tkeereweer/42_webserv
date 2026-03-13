#include "Response.hpp"
#include "Client.hpp"
#include "Server.hpp"

std::string	resolvePath(std::string relPath);

/*******************************************************************************
*						CTOR/DTOR
*******************************************************************************/

Response::Response(void):
	_returnCode(0),
	_toRead(0),
	_responseComplete(false),
    _sendTimestamp(0)
{
}

Response::~Response(void){}

Response    &Response::operator=(Response const &rhs)
{
	if (this != &rhs)
	{
		this->_protocol = rhs._protocol;
		this->_returnCode = rhs._returnCode;
		this->_reasonPhrase = rhs._reasonPhrase;
		this->_allow = rhs._allow;

		this->_contentEncoding = rhs._contentEncoding;
		this->_contentLength = rhs._contentLength;
		this->_toRead = rhs._toRead;
		this->_contentType = rhs._contentType;
		this->_location = rhs._location;
		this->_setCookie = rhs._setCookie;

		this->_entityBody = rhs._entityBody;
		this->_bodyFilepath = rhs._bodyFilepath;

		this->_rawResponse = rhs._rawResponse;
		this->_responseComplete = rhs._responseComplete;
		this->_sendTimestamp = rhs._sendTimestamp;
	}
	return (*this);
}

Response::Response(Response const &src)
{
	*this = src;
}


/*******************************************************************************
*						GET / SET
*******************************************************************************/
//getters

std::string &Response::getRawResponse(void)
{
	return (this->_rawResponse);
}

std::string Response::getContentLength(void) const
{
	return (this->_contentLength);
}

std::string 	Response::getContentType(void) const
{
	return (this->_contentType);
}

std::time_t	&Response::getSendTimestamp(void)
{
	return (this->_sendTimestamp);
}

bool	Response::getRespFlag(void) const
{
	return (this->_responseComplete);
}

size_t	Response::getToRead(void) const
{
	return (this->_toRead);
}

std::string	&Response::getEntityBody(void)
{
	return (this->_entityBody);
}

short	Response::getReturnCode(void) const
{
	return (this->_returnCode);
}

//setters

void	Response::setSendTimestamp(std::time_t timestamp)
{
	this->_sendTimestamp = timestamp;
}

void	Response::setContentLength(std::string length)
{
	this->_contentLength = length;
}

void	Response::setContentType(std::string type)
{
	this->_contentType = type;
}

void	Response::setToRead(size_t bytes)
{
	this->_toRead = bytes;
}

void	Response::setReturnCode(short code)
{
	this->_returnCode = code;
}

void	Response::setSetCookie(const std::string &setCookie)
{
    this->_setCookie = setCookie;
}

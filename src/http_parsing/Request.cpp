#include "../../include/Request.hpp"

Request::Request(void): _method(EMPTY), _contentLength(0), _reqComplete(false){}

Request::Request(Request const &src)
{
	*this = src;
}

Request	&Request::operator=(Request const &rhs)
{
	if (this != &rhs)
	{
		this->_method = rhs._method;
		this->_URI = rhs._URI;
		this->_HTTPVersion = rhs._HTTPVersion;
		this->_contentEncoding = rhs._contentEncoding;
		this->_contentLength = rhs._contentLength;
		this->_contentType = rhs._contentType;
		this->_cookies = rhs._cookies;
		this->_bodyFilename = rhs._bodyFilename;
	}
	return (*this);
}

Request::~Request(void)
{
	if (this->_bodyFilename != "")
		unlink(this->_bodyFilename.c_str());
}

//Returns -1 if request complete, 0 if expecting more headers, # of bytes to read until end of header.
//Don't call again if -1 or >0
//Consumes data such that data=="leftover after CRLF" after each call.
int	Request::lexRawData(std::string &data)
{
	if (data == "")
		throw(std::runtime_error("empty data field"));

	this->_lexInput(data);
	data.clear();
	return (_requestEval(data));
}

t_method const	&Request::getMethod(void) const
{
	return (this->_method);
}

std::string const	&Request::getURI(void) const
{
	return (this->_URI);
}

std::string const   &Request::getHTTPVersion(void) const
{
	return (this->_HTTPVersion);
}

std::string const   &Request::getContentEncoding(void) const
{
	return (this->_contentEncoding);
}

long long const		&Request::getContentLength(void) const
{
	return (this->_contentLength);
}

std::string const   &Request::getContentType(void) const
{
	return (this->_contentType);
}

std::string const	&Request::getCookies(void) const
{
	return (this->_cookies);
}

std::string const	&Request::getBodyFilename(void) const
{
	return (this->_bodyFilename);
}

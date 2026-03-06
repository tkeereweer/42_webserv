#include "../../include/Request.hpp"

Request::Request(void):
	_method(EMPTY),
	_contentLength(0),
	_reqComplete(false),
	_reqLineValid(false), 
	_reqHeadersValid(false),
	_bytesRead(0)
	{
		this->_recvTimestamp.tv_sec = 0;
		this->_recvTimestamp.tv_usec = 0;
	}

Request::Request(Request const &src)
{
	*this = src;
}

std::ostream    &operator<<(std::ostream &stream, Request const &rhs)
{
  	stream << rhs.getMethod() << std::endl;
	stream << rhs.getURI() << std::endl;
	stream << rhs.getHTTPVersion() << std::endl;
	stream << rhs.getContentEncoding() << std::endl;
	stream << rhs.getContentLength() << std::endl;
	stream << rhs.getContentType() << std::endl;
	stream << rhs.getCookies() << std::endl;
	if (rhs.getBodyFilename() != "")
	{
		stream << rhs.getBodyFilename() << std::endl;
		std::ifstream file(rhs.getBodyFilename().c_str());
		stream << "file content: '";
		std::string buf;
		while (getline(file, buf))
			stream << buf;
		stream << "'" << std::endl;
	} 
	return (stream);
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
		this->_reqComplete = rhs._reqComplete;
		this->_reqHeadersValid = rhs._reqHeadersValid;
		this->_reqLineValid = rhs._reqLineValid;
		this->_tokenList = rhs._tokenList;
		this->_recvTimestamp = rhs._recvTimestamp;
		this->_queryParam = rhs._queryParam;
		this->_bytesRead = rhs._bytesRead;
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

	std::list<t_reqToken>::reverse_iterator	rit = this->_tokenList.rbegin();
	while (rit->type != CRLF && rit != this->_tokenList.rend())
		rit++;
	if (rit == this->_tokenList.rend())
		return (0);
	//throw exception only if parsing interupted on bad grammar
	try
	{   
		_parse();
	}
	catch(std::exception const &e)
	{
		throw(std::runtime_error(e.what()));
	}

	if (this->_reqComplete)
		return (-1);
	data.clear(); //issue here where we have deleted token list and we then clear data, losing info

	//pop back in data last potentially unread token then pop_back() when headers not complete
	if (!this->_reqHeadersValid && !this->_tokenList.empty() && (this->_tokenList.back().type == WORD || this->_tokenList.back().type == SPACE))
	{
		data.append(this->_tokenList.back().val.begin(), this->_tokenList.back().val.end());
		this->_tokenList.pop_back();
	}

	if (!(this->_reqHeadersValid && this->_reqLineValid))
		return (0);
	else
		return (this->_contentLength - this->_bytesRead);
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

struct timeval Request::getRecvTimestamp(void) const
{
	return (this->_recvTimestamp);
}

bool  Request::getReqFlag(void) const
{
	return (this->_reqComplete);
}

std::string	&Request::getQueryParam(void)
{
	return (this->_queryParam);
}

bool	Request::getHeaderFlag(void) const
{
	return (this->_reqHeadersValid);
}

long long	Request::getBytesRead(void) const
{
	return (this->_bytesRead);
}

//setters
void    Request::setRecvTimestamp(struct timeval time)
{
	this->_recvTimestamp = time;
}

void	Request::setURI(std::string path)
{
	this->_URI = path;
}
void	Request::setQueryParam(std::string params)
{
	this->_queryParam = params;
}

void	Request::setReqFlag(bool state)
{
	this->_reqComplete = state;
}

void	Request::addBytesRead(long long size)
{
	this->_bytesRead += size;
}



#include "../include/Response.hpp"
#include <fstream>
#include <vector>
#include <algorithm>
#include <sstream>

Response::Response(void): _returnCode(0), _responseComplete(false){}

Response::~Response(void){}

Response    &Response::operator=(Response const &rhs)
{
	if (this != &rhs)
	{
		this->_contentEncoding = rhs._contentEncoding;
		this->_contentLength = rhs._contentLength;
		this->_contentType = rhs._contentType;
		this->_location = rhs._location;
		this->_entityBody = rhs._entityBody;
		this->_returnCode = rhs._returnCode;
		this->_protocol = rhs._protocol;
		this->_reasonPhrase = rhs._reasonPhrase;
		this->_responseComplete = rhs._responseComplete;
		this->_setCookie = rhs._setCookie;
	}
	return (*this);
}

Response::Response(Response const &src)
{
	*this = src;
}

//dedicated function for 
void    Response::build405Response(bool getAllowed, bool postAllowed, bool deleteAllowed)
{
	this->_protocol = "HTTP/1.0";
	this->_returnCode = 405;
	this->_reasonPhrase = "Method Not Allowed";
	if (getAllowed || postAllowed || deleteAllowed)
		this->_allow = "Allow: ";
	if (getAllowed)
		this->_allow += "GET";
	if (postAllowed)
	{
		if (getAllowed)
			this->_allow += ", ";
		this->_allow += "POST";
	}
	if (deleteAllowed)
	{
		if (getAllowed || postAllowed)
			this->_allow += ", ";
		this->_allow += "DELETE";
	}
	return (_buildRawResponse());
}

//call dedicated function for 405
void    Response::buildErrorResponse(short code)
{
	this->_protocol = "HTTP/1.0";
	switch (code)
	{
		case (400):
			this->_returnCode = 400;
			this->_reasonPhrase = "Bad Request";
			break ;
		case (403):
			this->_returnCode = 403;
			this->_reasonPhrase = "Forbidden";
			break ;
		case (404):
			this->_returnCode = 404;
			this->_reasonPhrase = "Not Found";
			break ;
		case (408):
			this->_returnCode = 408;
			this->_reasonPhrase = "Request Timeout";
			break ;
		case (411):
			this->_returnCode = 411;
			this->_reasonPhrase = "Length Required";
			break ;
		case (413):
			this->_returnCode = 413;
			this->_reasonPhrase = "Payload Too Large";
			break ;
		case (500):
			this->_returnCode = 500;
			this->_reasonPhrase = "Internal Server Error";
			break ;
		case (503):
			this->_returnCode = 503;
			this->_reasonPhrase = "Service Unavailable";
			break ;
		default:
			throw (std::runtime_error("no matching error code handled"));
	}
	return (_buildRawResponse());
}

void	Response::_buildRawResponse(void)
{
	this->_rawResponse += this->_protocol;
	this->_rawResponse += " ";
	this->_rawResponse += this->_returnCode;
	this->_rawResponse += " ";
	this->_rawResponse += this->_reasonPhrase;
	this->_rawResponse += "\r\n";
	if (!this->_location.empty())
	{
		this->_rawResponse += "Location: ";
		this->_rawResponse += this->_location;
		this->_rawResponse += "\r\n";
	}
	if (this->_contentEncoding != "")
	{
		this->_rawResponse += "Content-Encoding: ";
		this->_rawResponse += this->_contentEncoding;
		this->_rawResponse += "\r\n";
	}
	if (this->_contentLength != "")
	{
		this->_rawResponse += "Content-Length: ";
		this->_rawResponse += this->_contentLength;
		this->_rawResponse += "\r\n";
	}
	if (this->_contentType != "")
	{
		this->_rawResponse += "Content-Type: ";
		this->_rawResponse += this->_contentType;
		this->_rawResponse += "\r\n";
	}
	if (this->_setCookie != "")
	{
		this->_rawResponse += "Set-Cookie: ";
		this->_rawResponse += this->_setCookie;
		this->_rawResponse += "\r\n";
	}
	this->_rawResponse += "\r\n";
	if (this->_entityBody != "")
		this->_rawResponse += this->_entityBody;
	this->_responseComplete = true;
	return ;
}

//read the right ressource, set content type, content length and encoding and write in rawPath
//if CGI handling or cookie setup, do here
void    Response::buildRouteResponse(std::string localPath)
{
	size_t			readSize = 1024 * 1024; //1MB buffer
	std::ifstream	file(localPath.c_str(), std::ios::binary); //last flag to preserve integrity ?
	if (!file.is_open())
		return (buildErrorResponse(500));
	//most memory efficient approach for large files
	//should we do "write" chunking of size == max_readable_chunk_per_TCP_packet ? THen use flags like for reading ?
	std::vector<char> buffer(readSize);
	long long	size = 0;
	while (file.read(&buffer[0], readSize))
	{
		if (this->_contentLength == "")
			this->_contentLength = "Content-Lenght: ";
		size += file.gcount(); //number of bytes read
		this->_entityBody.insert(this->_entityBody.end(), buffer.begin(), buffer.end());
		buffer.clear();
	}
	std::stringstream	sstr;
	sstr << size;
	this->_contentLength += sstr.str();
	//set to empty for now as it is the server's responsibility to encode or not. 
	//content length would refer to the encoded length.
	this->_contentEncoding = "";
	//for now, only type handled ?
	this->_contentType = "text/html";
	this->_returnCode = 200;
	this->_protocol = "HTTP/1.0";
	this->_reasonPhrase = "OK";
	return (_buildRawResponse());
}

void    Response::buildRedirResponse(std::string redirPath)
{
	this->_protocol = "HTTP/1.0";
	this->_returnCode = 302;
	this->_reasonPhrase = "Found";
	this->_location = redirPath;
	return (_buildRawResponse());
}
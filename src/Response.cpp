#include "../include/Response.hpp"
#include "../include/Client.hpp"
#include "../include/Server.hpp"
// #include <unistd.h>
// #include <fcntl.h>


Response::Response(void):
	_returnCode(0),
	_toRead(0),
	_responseComplete(false)
{
	this->_sendTimestamp.tv_sec = this->_sendTimestamp.tv_usec = 0;
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
		this->_sendTimestamp.tv_sec = rhs._sendTimestamp.tv_sec;
		this->_sendTimestamp.tv_usec = rhs._sendTimestamp.tv_usec;
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
	return (buildRawResponse());
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
			this->_bodyFilepath = "/home/mturgeon/rank5/webserv/data/www/pages/errors/400.html"; //filepath temporary cuz I suck and can't make relative path work...
			break ;
		case (403):
			this->_returnCode = 403;
			this->_reasonPhrase = "Forbidden";
			this->_bodyFilepath = "/home/mturgeon/rank5/webserv/data/www/pages/errors/403.html";
			break ;
		case (404):
			this->_returnCode = 404;
			this->_reasonPhrase = "Not Found";
			this->_bodyFilepath = "/home/mturgeon/rank5/webserv/data/www/pages/errors/404.html";
			break ;
		case (408):
			this->_returnCode = 408;
			this->_reasonPhrase = "Request Timeout";
			this->_bodyFilepath = "/home/mturgeon/rank5/webserv/data/www/pages/errors/408.html";
			break ;
		case (409):
			this->_returnCode = 409;
			this->_reasonPhrase = "Conflict";
			this->_bodyFilepath = "/home/mturgeon/rank5/webserv/data/www/pages/errors/409.html";
			break ;
		case (411):
			this->_returnCode = 411;
			this->_reasonPhrase = "Length Required";
			this->_bodyFilepath = "/home/mturgeon/rank5/webserv/data/www/pages/errors/411.html";
			break ;
		case (413):
			this->_returnCode = 413;
			this->_reasonPhrase = "Payload Too Large";
			this->_bodyFilepath = "/home/mturgeon/rank5/webserv/data/www/pages/errors/413.html";
			break ;
		case (500):
			this->_returnCode = 500;
			this->_reasonPhrase = "Internal Server Error";
			struct stat buf;
			if (stat("/home/mturgeon/rank5/webserv/data/www/pages/errors/500.html", &buf) == 0)
			{
				std::cout << "500 error page file doesnt exist\n";
				this->_bodyFilepath = "";
				break;
			} //get out of infinite loop if error page doesn't exist
			this->_bodyFilepath = "/home/mturgeon/rank5/webserv/data/www/pages/errors/500.html";
			break ;
		case (502):
			this->_returnCode = 502;
			this->_reasonPhrase = "Bad Gateway";
			this->_bodyFilepath = "/home/mturgeon/rank5/webserv/data/www/pages/errors/502.html";
			break ;
		case (503):
			this->_returnCode = 503;
			this->_reasonPhrase = "Service Unavailable";
			this->_bodyFilepath = "/home/mturgeon/rank5/webserv/data/www/pages/errors/503.html";
			break ;
		default:
			throw (std::runtime_error("no matching error code handled"));
	}
	return (buildRawResponse());
}

void	Response::buildRawResponse(void)
{
	std::stringstream returnCodeStr;

	this->_rawResponse += this->_protocol;
	this->_rawResponse += " ";
	returnCodeStr << this->_returnCode;
	this->_rawResponse += returnCodeStr.str();
	this->_rawResponse += " ";
	this->_rawResponse += this->_reasonPhrase;
	this->_rawResponse += "\r\n";
	if (!this->_bodyFilepath.empty())
		_writeFileToResponse(this->_bodyFilepath);
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
	this->_toRead += this->_rawResponse.size();
	if (this->_entityBody != "")
		this->_rawResponse += this->_entityBody;
	this->_responseComplete = true;
	return ;
}

void    Response::_writeFileToResponse(std::string filepath)
{
	size_t			readSize = 1024 * 1024; //1MB buffer
	std::ifstream	file(filepath.c_str(), std::ios::binary); //allows opening of file with binary data like jpegs
	if (!file.is_open())
	{
		std::cout << "file: " << filepath << " failed to open" <<std::endl;
		throw (std::runtime_error("500"));
	}
	//most memory efficient approach for large files
	//should we do "write" chunking of size == max_readable_chunk_per_TCP_packet ? THen use flags like for reading ?
	std::vector<char> buffer(readSize);
	long long	size = 0;
	file.read(&buffer[0], readSize);
	while (file.gcount() != 0)
	{
		size += file.gcount(); //number of bytes read
		this->_entityBody.insert(this->_entityBody.end(), buffer.begin(), buffer.end());
		buffer.clear();
		file.read(&buffer[0], readSize);
	}
	std::stringstream	sstr;
	sstr << size;
	this->_contentLength += sstr.str();
	this->_toRead = size;
	//set to empty for now as it is the server's responsibility to encode or not.
	//content length would refer to the encoded length.
	this->_contentEncoding = "";
	//for now, only text/html or text/css handled with this method
	std::string extension(&filepath[filepath.find_last_of(".") + 1]);
	this->_contentType = "text/";
	this->_contentType += extension;
	this->_contentType += "; charset=utf-8";
	return ;
}

//read the right ressource, set content type, content length and encoding and write in rawPath
//if CGI handling or cookie setup, do here
void    Response::buildRouteResponse(std::string localPath)
{
	_writeFileToResponse(localPath);
	this->_returnCode = 200;
	this->_protocol = "HTTP/1.0";
	this->_reasonPhrase = "OK";
	return (buildRawResponse());
}

void    Response::buildRedirResponse(std::string redirPath)
{
	this->_protocol = "HTTP/1.0";
	this->_returnCode = 302;
	this->_reasonPhrase = "Found";
	this->_location = redirPath;
	return (buildRawResponse());
}

void	Response::buildPostResponse(std::string createdFile)
{
	std::stringstream	stream;

	this->_returnCode = 201;
	this->_protocol = "HTTP/1.0";
	this->_reasonPhrase = "Created";
	this->_entityBody.append(createdFile);
	stream << createdFile.length();
	this->_contentLength = stream.str();
	this->_toRead = createdFile.length();
	return (buildRawResponse());
}

void    Response::buildGetCGIResponse(Client &client, Location *loc, int epollFD, Server &server, std::string scriptPath)
{
	server.getCgiVec().push_back(CGI(client.getRequest().getQueryParam(), server.getParentEnv(), client, loc, scriptPath));
	server.addCgiToEpoll(server.getCgiVec().back(), epollFD);
	client.setCgiResponseState(1); //ongoing
    this->_protocol = "HTTP/1.0";
    this->_returnCode = 200;
    this->_reasonPhrase = "OK";
    return ;
}

void Response::buildPostCgiResponse(Client &client, Location *loc, int epollFD, Server &server, std::string scriptPath)
{
	server.getCgiVec().push_back(CGI(server.getParentEnv(), client, loc, scriptPath));
	server.addCgiToEpoll(server.getCgiVec().back(), epollFD);
	client.setCgiResponseState(1);
    this->_protocol = "HTTP/1.0";
    this->_returnCode = 200;
    this->_reasonPhrase = "OK";
    return ;
}

void	Response::buildDelResponse(Client &client, std::string& path)
{
	std::string	filename = client.getRequest().getURI();

	std::cout<<"path: "<< path << std::endl;
	std::cout<<"filename: "<< filename << std::endl;

    errno = 0;
	if (unlink(path.c_str()) == -1)
	{
		if (errno == EACCES || errno == EPERM || errno == EROFS)
			return (this->buildErrorResponse(403));
		else if (errno == EBUSY)
			return (this->buildErrorResponse(409));
		else
			return (this->buildErrorResponse(404));
	}

	else
		_returnCode = 200; //204 could do

	this->_protocol = "HTTP/1.0";
	this->_reasonPhrase = "File has been deleted";

	return (buildRawResponse());
}

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

struct timeval	&Response::getSendTimestamp(void)
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

//setters

void	Response::setSendTimestamp(struct timeval timestamp)
{
	this->_sendTimestamp.tv_sec = timestamp.tv_sec;
	this->_sendTimestamp.tv_usec = timestamp.tv_usec;
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

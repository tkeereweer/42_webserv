#include "../include/Response.hpp"
#include "../include/Client.hpp"
#include "../include/Server.hpp"
// #include <unistd.h>
// #include <fcntl.h>

std::string	resolvePath(std::string relPath);

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
void    Response::build405Response(bool getAllowed, bool postAllowed, bool deleteAllowed, Server *server, Location *loc)
{
	std::map<int, std::string>::iterator it;

	this->_protocol = "HTTP/1.0";
	this->_returnCode = 405;
	this->_reasonPhrase = "Method Not Allowed";
	if (loc && (it = loc->getErrorPages().find(405)) != loc->getErrorPages().end())
		this->_bodyFilepath = server->buildPath("/" + it->second, loc);
	else if (loc && server && (it = server->getErrorPages().find(405)) != server->getErrorPages().end())
		this->_bodyFilepath = server->buildPath("/" + it->second, loc);
	else
		this->_bodyFilepath = resolvePath("data/www/default-errors/405.html");
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
void    Response::buildErrorResponse(short code, Server *server, Location *loc)
{
	std::map<int, std::string>::iterator it;

	this->_protocol = "HTTP/1.0";
	this->_bodyFilepath.clear();
	if (loc && (it = loc->getErrorPages().find(code)) != loc->getErrorPages().end()) //problem here when loc is NULL
		this->_bodyFilepath = server->buildPath("/" + it->second, loc);
	else if (server && (it = server->getErrorPages().find(code)) != server->getErrorPages().end())
		this->_bodyFilepath = server->buildPath("/" + it->second, loc);
	switch (code)
	{
		case (400):
			this->_returnCode = 400;
			this->_reasonPhrase = "Bad Request";
			if (this->_bodyFilepath.empty())
				this->_bodyFilepath = resolvePath("data/www/default-errors/400.html");
			break ;
		case (403):
			this->_returnCode = 403;
			this->_reasonPhrase = "Forbidden";
			if (this->_bodyFilepath.empty())
				this->_bodyFilepath = resolvePath("data/www/default-errors/403.html");
			break ;
		case (404):
			this->_returnCode = 404;
			this->_reasonPhrase = "Not Found";
			if (this->_bodyFilepath.empty())
				this->_bodyFilepath = resolvePath("data/www/default-errors/404.html");
			break ;
		case (408):
			this->_returnCode = 408;
			this->_reasonPhrase = "Request Timeout";
			if (this->_bodyFilepath.empty())
				this->_bodyFilepath = resolvePath("data/www/default-errors/408.html");
			break ;
		case (409):
			this->_returnCode = 409;
			this->_reasonPhrase = "Conflict";
			if (this->_bodyFilepath.empty())
				this->_bodyFilepath = resolvePath("data/www/default-errors/409.html");
			break ;
		case (411):
			this->_returnCode = 411;
			this->_reasonPhrase = "Length Required";
			if (this->_bodyFilepath.empty())
				this->_bodyFilepath = resolvePath("data/www/default-errors/411.html");
			break ;
		case (413):
			this->_returnCode = 413;
			this->_reasonPhrase = "Payload Too Large";
			if (this->_bodyFilepath.empty())
				this->_bodyFilepath = resolvePath("data/www/default-errors/413.html");
			break ;
        case (414):
			this->_returnCode = 414;
			this->_reasonPhrase = "Payload Too Large";
			if (this->_bodyFilepath.empty())
				this->_bodyFilepath = resolvePath("data/www/default-errors/414.html");
			break ;
		case (500):
			this->_returnCode = 500;
			this->_reasonPhrase = "Internal Server Error";
			struct stat	buf;
			if (stat(resolvePath("data/www/default-errors/500.html").c_str(), &buf) != 0)
			{
				std::cout << "500 error page file doesnt exist\n"; 
				this->_bodyFilepath = "";
				break;
			}  //get out of infinite loop if error page doesn't exist
			if (this->_bodyFilepath.empty())
				this->_bodyFilepath = resolvePath("data/www/default-errors/500.html");
			break ;
		case (502):
			this->_returnCode = 502;
			this->_reasonPhrase = "Bad Gateway";
			if (this->_bodyFilepath.empty())
				this->_bodyFilepath = resolvePath("data/www/default-errors/502.html");
			break ;
		case (503):
			this->_returnCode = 503;
			this->_reasonPhrase = "Service Unavailable";
			if (this->_bodyFilepath.empty())
				this->_bodyFilepath = resolvePath("data/www/default-errors/503.html");
			break ;
        case (505):
			this->_returnCode = 505;
			this->_reasonPhrase = "Service Unavailable";
			if (this->_bodyFilepath.empty())
				this->_bodyFilepath = resolvePath("data/www/default-errors/505.html");
			break ;
		default:
			throw (std::runtime_error("no matching error code handled"));
	}
	return (buildRawResponse());
}

void	Response::buildRawResponse(void)
{
	std::stringstream returnCodeStr;

	this->_rawResponse += this->_protocol + " ";
	returnCodeStr << this->_returnCode;
	this->_rawResponse += returnCodeStr.str() + " " + this->_reasonPhrase + "\r\n";
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
void    Response::buildRouteResponse(std::string localPath, Server *server, Location *loc)
{
	if (access(localPath.c_str(), R_OK) == -1)
		return (buildErrorResponse(404, server, loc));
	_writeFileToResponse(localPath);
	this->_returnCode = 200;
	this->_protocol = "HTTP/1.0";
	this->_reasonPhrase = "OK";
	return (buildRawResponse());
}

void    Response::buildRedirResponse(int redirCode, std::string redirPath)
{
	this->_protocol = "HTTP/1.0";
	this->_returnCode = redirCode;
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
	this->_reasonPhrase = "OK";
	return ;
}

void Response::buildPostCgiResponse(Client &client, Location *loc, int epollFD, Server &server, std::string scriptPath)
{
	server.getCgiVec().push_back(CGI(server.getParentEnv(), client, loc, scriptPath));
	server.addCgiToEpoll(server.getCgiVec().back(), epollFD);
	client.setCgiResponseState(1);
	this->_protocol = "HTTP/1.0";
	this->_reasonPhrase = "OK";
	return ;
}

void	Response::buildDelResponse(Client &client, std::string& path, Server *server, Location *loc)
{
	std::string	filename = client.getRequest().getURI();

	std::cout<<"path: "<< path << std::endl;
	std::cout<<"filename: "<< filename << std::endl;

	errno = 0;
	if (unlink(path.c_str()) == -1)
	{
		if (errno == EACCES || errno == EPERM || errno == EROFS)
			return (buildErrorResponse(403, server, loc));
		else if (errno == EBUSY)
			return (buildErrorResponse(409, server, loc));
		else
			return (buildErrorResponse(400, server, loc));
	}

	else
		_returnCode = 200; //204 could do

	this->_protocol = "HTTP/1.0";
	this->_reasonPhrase = "File has been deleted";

	return (buildRawResponse());
}

void	Response::buildDirectoryListingResponse(std::string &dir, Server *server, Location *loc)
{
	std::string	pageName;
	std::string	content;
	std::string dirDisplayed = dir; //not full path but actual URI requested
	if (dir[dir.size() - 1] == '/')
		dirDisplayed.erase(dirDisplayed.size() - 1);
	dirDisplayed = std::string(dirDisplayed.begin() + dirDisplayed.find_last_of("/") + 1, dirDisplayed.end());
	if (dirDisplayed == "")
		dirDisplayed = "/";
	this->_protocol = "HTTP/1.0";
	this->_returnCode = 200;
	this->_reasonPhrase = "OK";
	this->_contentType = "text/html; charset=utf-8";
	content = "<!DOCTYPE html><html><head><title>Index of " + dirDisplayed + "</title></head><body><h1>Index of " + dirDisplayed + "</h1><hr><pre>\n";
	if (dirDisplayed != "/") //don't do the following if there are no pages up the tree
		content += "<a href=\"../\">../</a>\n";

	errno = 0;
	DIR *tmp = opendir(dir.c_str());
	if (tmp == NULL)
		return (buildErrorResponse(500, server, loc));
	struct dirent *name = readdir(tmp);
	if (!name && errno != 0)
		return (buildErrorResponse(500, server, loc));
	while (name)
	{
		std::string temp = name->d_name;
		std::string::iterator start = temp.begin() + temp.find_last_of("/") + 1;
		pageName = std::string(start, temp.end()); 
		if (pageName != "." && pageName != "..")
			content += "<a href=\"" + pageName + "\">" + pageName + "</a>\n";
		name = readdir(tmp);
	}

	content += "</pre><hr></body></html>";
	std::stringstream sstr;
	sstr << content.size();
	this->_contentLength = sstr.str();
	this->_toRead = content.size();
	this->_entityBody = content;
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

short	Response::getReturnCode(void) const
{
	return (this->_returnCode);
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

void	Response::setSetCookie(const std::string &setCookie)
{
    this->_setCookie = setCookie;
}

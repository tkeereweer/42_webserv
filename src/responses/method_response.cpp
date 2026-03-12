#include "Response.hpp"
#include "Server.hpp"

/*******************************************************************************
*						REDIR
*******************************************************************************/

void    Response::buildRedirResponse(int redirCode, std::string redirPath)
{
	this->_protocol = "HTTP/1.0";
	this->_returnCode = redirCode;
	this->_reasonPhrase = "Found";
	this->_location = redirPath;
	return (buildRawResponse());
}

/*******************************************************************************
*						METHODS
*******************************************************************************/

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
        if (errno == ENOENT)
            return (buildErrorResponse(404, server, loc));
		else if (errno == EACCES || errno == EPERM || errno == EROFS)
			return (buildErrorResponse(403, server, loc));
		else if (errno == EBUSY)
			return (buildErrorResponse(409, server, loc));
		else
			return (buildErrorResponse(400, server, loc));
	}

	else
		_returnCode = 204; //sometimes 200

	this->_protocol = "HTTP/1.0";
	this->_reasonPhrase = "File has been deleted";

	return (buildRawResponse());
}


/*******************************************************************************
*						DIR LISTING
*******************************************************************************/

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

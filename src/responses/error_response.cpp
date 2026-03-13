#include "Response.hpp"
#include "Client.hpp"
#include "Server.hpp"

std::string	resolvePath(std::string relPath);


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

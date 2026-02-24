#include "../include/Server.hpp"
#include <stdexcept>
#include <sys/stat.h>

/*******************************************************************************
*						CTOR/DTOR
*******************************************************************************/

Server::Server(void): Config() {}

Server::Server(Server const &src):
	Config(src),
	_name(src._name),
	_sockets(src._sockets),
	_locations(src._locations)
{}

Server	&Server::operator=(Server const &rhs)
{
	if (this != &rhs)
	{
		this->_sockets = rhs._sockets;
		this->_locations = rhs._locations;
		this->_name = rhs._name;
		this->_acceptGET = rhs._acceptGET;
		this->_acceptPOST = rhs._acceptPOST;
		this->_acceptDELETE = rhs._acceptDELETE;
		this->_root = rhs._root;
		this->_autoIndex = rhs._autoIndex;
		this->_index = rhs._index;
		this->_maxBodySizeClientReq = rhs._maxBodySizeClientReq;
		this->_errorPages = rhs._errorPages;
		this->_redirect = rhs._redirect;
	}
	return (*this);
}

Server::~Server(void) {}


/*******************************************************************************
*						GET/SET
*******************************************************************************/

std::string	Server::getName(void) const
{
	return (this->_name);
}

std::vector<t_socket>	&Server::getSockets(void)
{
	return (this->_sockets);
}

std::vector<Location>	&Server::getLocations(void)
{
	return (this->_locations);
}

void	Server::setName(std::string name)
{
	this->_name = name;
}

void	Server::addSocket(t_socket socket)
{
	this->_sockets.push_back(socket);
}

void	Server::addLocation(Location location)
{
	this->_locations.push_back(location);
}

/*******************************************************************************
*						DISPATCHER
*******************************************************************************/

int	Server::matchLocation(std::string URI) const
{
	int	matched = -1;

	for (std::size_t i = 0; i < this->_locations.size(); i++)
	{
		long unsigned int	j = 0;
		for (; j < this->_locations[i].getPath().length(); j++)
		{
			if (j >= URI.length() || URI[j] != this->_locations[i].getPath()[j])
				break;
		}
		if (j == this->_locations[i].getPath().length() &&
			this->_locations[i].getPath().length() > this->_locations[matched].getPath().length())
			matched = i;
	}
	return (matched);
}

bool	Server::isMethodAllowed(t_method method, Location &loc) const
{
	if (method == GET && loc.getAcceptGET() != false && this->getAcceptGET() != false)
		return (true);
	else if (method == POST && loc.getAcceptPOST() != false && this->getAcceptPOST() != false)
		return (true);
	else if (method == DELETE && loc.getAcceptDELETE() != false && this->getAcceptDELETE() != false)
		return (true);
	else
		return (false);
}

void	Server::dispatchRequest(Client &client)
{
	Request	    &req = client.getRequest();
	Response    &resp = client.getResponse();
	int	locIdx = matchLocation(req.getURI());

	if (locIdx == -1)
		return (resp.buildErrorResponse(404));
	Location	&loc = this->_locations[locIdx];
	if (!isMethodAllowed(req.getMethod(), loc))
		return (resp.build405Response(loc.getAcceptGET(), loc.getAcceptPOST(), loc.getAcceptDELETE()));
	if (req.getMethod() == GET)
		handleGET(client, loc);
	// else if (req.getMethod() == POST)
	// 	handlePOST(client, loc);
	// else
	// 	handleDELETE(client, loc);
}

// void	Server::handleError(Client &client, Location &loc, short code) const
// {
// 	std::map<int, std::string>::const_iterator	it;
// 	// Response	&resp = client.getResponse();

// 	if ((it = loc.getErrorPages().find(code)) != loc.getErrorPages().end())
// 	{
// 		// return custom error page (it->second)
// 	}
// 	else if ((it = this->_errorPages.find(code)) != this->_errorPages.end())
// 	{
// 		// return custom error page (it->second)
// 	}
// 	else
// 	{
// 		resp.
// 	}
// }

/*******************************************************************************
*						HANDLE GET/POST/DELETE
*******************************************************************************/

static int	isDir(char const *path)
{
   struct stat	statbuf;
   if (stat(path, &statbuf) != 0)
	   return (0);
   return S_ISDIR(statbuf.st_mode);
}

std::string	Server::buildPath(std::string URI, Location &loc) const
{
	std::string	path = ".";
	if (!loc.getRoot().empty())
		path.append(loc.getRoot());
	else if (!this->_root.empty())
		path.append(this->_root);
	path.append(URI);
	return (path);
}

void	Server::handleDir(Client &client, Location &loc, std::string dir) const
{
	std::string	path;

	if (!loc.getIndex().empty()) //build location path first if it exists
		return (client.getResponse().buildRouteResponse(buildPath(loc.getIndex(), loc)));
	else if (!this->_index.empty()) //else, build default location path for server
		return (client.getResponse().buildRouteResponse(buildPath(this->_index, loc)));
	if (!path.empty())
		return (client.getResponse().buildRouteResponse("/index.html")); //if failed, return homepage
	else if (loc.getAutoIndex() == 1 || (loc.getAutoIndex() != 0 && this->_autoIndex == 1))
	{
		std::cout << "directory listing, for now just /index.html" << std::endl;
		return (client.getResponse().buildRouteResponse("/index.html"));
		// return directory listing
	}
	dir.append("index.html");
	if (access(dir.c_str(), R_OK) == -1)
		return (client.getResponse().buildErrorResponse(403));
	else
		return (client.getResponse().buildRouteResponse("/index.html")); //is that what's supposed to happen ? I don't think I understood the right path...
}

void	Server::handleGET(Client &client, Location &loc) const
{
	Request	&req = client.getRequest();
	if (!loc.getRedir().second.empty())
		return (client.getResponse().buildRedirResponse(loc.getRedir().second));
	else if (!this->_redirect.second.empty())
		return (client.getResponse().buildRedirResponse(this->_redirect.second));
	std::string	path = buildPath(req.getURI(), loc);
	if (isDir(path.c_str()))
		return (handleDir(client, loc, path));
	if (access(path.c_str(), R_OK) == -1)
		return (client.getResponse().buildErrorResponse(404));
	return (client.getResponse().buildRouteResponse(loc.getPath()));
}


// void	Server::handlePOST(Client &client, Location &loc) const
// {
	
// }
#include "../include/Server.hpp"

/*******************************************************************************
*						CTOR/DTOR
*******************************************************************************/

Server::Server(void): Config(){}

Server::Server(std::vector<std::string> envp): Config(), _parentEnv(envp) {}

Server::Server(Server const &src):
	Config(src),
	_name(src._name),
	_sockets(src._sockets),
	_locations(src._locations),
	_parentEnv(src._parentEnv)
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
		this->_cgiVec = rhs._cgiVec;
		this->_parentEnv = rhs._parentEnv;
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

std::vector<CGI>		&Server::getCgiVec(void)
{
	return (this->_cgiVec);
}

std::vector<std::string>	Server::getParentEnv(void) const
{
	return (this->_parentEnv);
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
		//j is index of character in URI
		for (; j < this->_locations[i].getPath().length(); j++)
		{
			//breaks if location path > URI length or different char -> not matching
			if (j >= URI.length() || URI[j] != this->_locations[i].getPath()[j])
				break;
		}
		if (matched == -1)
			matched = i;
		//returns "longest" match block, aka most "precise" match block.
		else if (j == this->_locations[i].getPath().length() &&
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

static int	isDir(char const *path)
{
   struct stat	statbuf;
   if (stat(path, &statbuf) != 0)
	   return (0);
   return S_ISDIR(statbuf.st_mode);
}

void	Server::_getQueryParams(Request &req)
{
	if (req.getURI().find_first_of("?") != std::string::npos)
	{
		std::string 			URI = req.getURI();
		std::string::iterator	end = URI.begin() + req.getURI().find_first_of("?");
		std::string				relPath(URI.begin(), end);
		req.setURI(relPath);
		std::string				queryParam(end + 1, URI.end());
		req.setQueryParam(queryParam);		
	}
}

void	Server::dispatchRequest(Client &client, int epollFD)
{
	Request		&req = client.getRequest();
	Response	&resp = client.getResponse();

	_getQueryParams(req);
	int	locIdx = matchLocation(req.getURI());
	if (locIdx == -1)
		return (resp.buildErrorResponse(404));
	Location	&loc = this->_locations[locIdx];
	if (!loc.getRedir().second.empty())
		return (client.getResponse().buildRedirResponse(loc.getRedir().second));
	else if (!this->_redirect.second.empty())
		return (client.getResponse().buildRedirResponse(this->_redirect.second));
	if ((loc.getMaxBody() != -1 && req.getContentLength() > loc.getMaxBody())
		|| (this->_maxBodySizeClientReq != -1 && req.getContentLength() > this->_maxBodySizeClientReq))
		return (resp.buildErrorResponse(413));
	std::string	path = buildPath(req.getURI(), loc);
	if (isDir(path.c_str()))
		return (handleDir(client, loc, path));
	if (access(path.c_str(), R_OK) == -1)
	{
		std::cout << "access errno: " << strerror(errno) << std::endl;
		std::cout << "path: " << path << std::endl << "path.c_str(): " << path.c_str() << std::endl;
		return (client.getResponse().buildErrorResponse(404));
	}
	if (!isMethodAllowed(req.getMethod(), loc))
		return (resp.build405Response(loc.getAcceptGET(), loc.getAcceptPOST(), loc.getAcceptDELETE()));
	if (req.getMethod() == GET)
		handleGET(client, loc, path, epollFD);
	else if (req.getMethod() == POST)
		handlePOST(loc, client, path, this->_parentEnv, epollFD);
	// else
	// 	handleDELETE(client, loc);
}

/*******************************************************************************
*						HANDLE GET/POST/DELETE
*******************************************************************************/

std::string	Server::buildPath(std::string URI, Location &loc) const
{
    //this field should not end w/ a '/'
	std::string	path = "/home/mkeerewe/42/rank05/webserv_perso"; //I modified this from "." to absolute path for my machine because wtf is going on i can't make them work
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

void	Server::handleGET(Client &client, Location &loc, std::string path, int epollFD)
{
	try 
	{	
		Request	&req = client.getRequest();
        //handle GET Cgi
        if (req.getURI().find(".py") != std::string::npos || req.getURI().find(".php") != std::string::npos) //.php or any other handled cgi
			return (client.getResponse().buildGetCGIResponse(client, &loc, epollFD, *this, path)); //needs full path in there
		return (client.getResponse().buildRouteResponse(path));
	}
	catch (std::exception const &e)
	{
		std::cerr << e.what() << std::endl;
		return (client.getResponse().buildErrorResponse(500));
	}
}


void	Server::handlePOST(Location &loc, Client &client, std::string path, std::vector<std::string> serverEnv, int epollFD)
{
	try
	{
		this->_cgiVec.push_back(CGI(serverEnv, client, &loc, path)); 
		addCgiToEpoll(this->_cgiVec.back(), epollFD);
		client.setCgiResponseState(1);
		return (client.getResponse().buildPostCgiResponse());
	}
	catch(const std::exception& e)
	{
		return (client.getResponse().buildErrorResponse(500));
	}
}

void	Server::addCgiToEpoll(CGI &cgi, int epollFD) const
{
	epoll_event	ev;
	ev.events = EPOLLIN;
	ev.data.fd = cgi.getReadFD();
	fcntl(cgi.getReadFD(), F_SETFL, O_NONBLOCK);
	epoll_ctl(epollFD, EPOLL_CTL_ADD, cgi.getReadFD(), &ev);

	if (cgi.getWriteFD() != -1)
	{
		ev.events = EPOLLOUT;
		ev.data.fd = cgi.getWriteFD();
		fcntl(cgi.getWriteFD(), F_SETFL, O_NONBLOCK);
		epoll_ctl(epollFD, EPOLL_CTL_ADD, cgi.getWriteFD(), &ev);
	}
}
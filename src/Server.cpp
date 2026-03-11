#include "../include/Server.hpp"

std::string	resolvePath(std::string relPath);

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


	//when error already built, exit straight away
	if (resp.getRespFlag())
		return;
	_getQueryParams(req);
	int	locIdx = matchLocation(req.getURI());
	if (locIdx == -1)
		return (resp.buildErrorResponse(404, this, NULL));
	Location	&loc = this->_locations[locIdx];
	if (!loc.getRedir().second.empty())
		return (resp.buildRedirResponse(loc.getRedir().first, loc.getRedir().second));
	else if (!this->_redirect.second.empty())
		return (resp.buildRedirResponse(this->_redirect.first, this->_redirect.second));
	if ((loc.getMaxBody() != -1 && req.getContentLength() > loc.getMaxBody())
		|| (this->_maxBodySizeClientReq != -1 && req.getContentLength() > this->_maxBodySizeClientReq))
		return (resp.buildErrorResponse(413, this, &loc));
	std::string	path = buildPath(req.getURI(), loc);
	// if (access(path.c_str(), F_OK) == -1)
	// 	return (resp.buildErrorResponse(404, this, &loc)); //TODO: make a decision about this line
	if (isDir(path.c_str()) || *(path.rbegin()) == '/')
		return (handleDir(client, loc, path));
	if (!isMethodAllowed(req.getMethod(), loc))
		return (resp.build405Response(isMethodAllowed(GET, loc), isMethodAllowed(POST, loc), isMethodAllowed(DELETE, loc), this, &loc));
	if (req.getMethod() == GET)
		handleGET(client, loc, path, epollFD);
	else if (req.getMethod() == POST)
		handlePOST(loc, client, path, epollFD);
	else
		handleDELETE(loc, client, path);
}

/*******************************************************************************
*						HANDLE GET/POST/DELETE
*******************************************************************************/

std::string	Server::buildPath(std::string URI, Location &loc) const
{
	//this field should not end w/ a '/'
	std::string	path;
	if (!loc.getRoot().empty())
		path = resolvePath(loc.getRoot());
	else if (!this->_root.empty())
		path = resolvePath(this->_root);
	if (path == "")
		path = resolvePath(URI);
	else
		path.append(URI);
	return (path);
}


void	Server::handleDir(Client &client, Location &loc, std::string dir)
{
	if (!loc.getIndex().empty()) //build location path first if it exists
		return (client.getResponse().buildRouteResponse(buildPath("/" + loc.getIndex(), loc), this, &loc));
	else if (!this->_index.empty()) //else, build default location path for server
		return (client.getResponse().buildRouteResponse(buildPath("/" + this->_index, loc), this, &loc));
	else if (loc.getAutoIndex() == 1 || (loc.getAutoIndex() != 0 && this->_autoIndex == 1))
		return (client.getResponse().buildDirectoryListingResponse(dir, this, &loc));
	dir.append("index.html"); //TODO: verify that dir ends with a "/"
	std::string	path = buildPath(dir, loc);
	if (access(path.c_str(), R_OK) == -1)
		return (client.getResponse().buildErrorResponse(403, this, &loc));
	else
		return (client.getResponse().buildRouteResponse(path, this, &loc)); 
}

void	Server::handleGET(Client &client, Location &loc, std::string path, int epollFD)
{
	try
	{
		Request	&req = client.getRequest();
		if (access(path.c_str(), R_OK) == -1)
		{
			std::cout << "access errno: " << strerror(errno) << std::endl;
			std::cout << "path: " << path << std::endl << "path.c_str(): " << path.c_str() << std::endl;
			return (client.getResponse().buildErrorResponse(404, this, &loc));
		}
		//handle GET Cgi
		if (req.getURI().find(".py") != std::string::npos || req.getURI().find(".php") != std::string::npos) //.php or any other handled cgi
			return (client.getResponse().buildGetCGIResponse(client, &loc, epollFD, *this, path)); //needs full path in there
		return (client.getResponse().buildRouteResponse(path, this, &loc));
	}
	catch (std::exception const &e)
	{
		std::cerr << e.what() << std::endl;
		return (client.getResponse().buildErrorResponse(500, this, &loc));
	}
}

void	Server::uploadFile(Location &loc, Client &client)
{
	Request			&req = client.getRequest();
	std::ifstream	ifs;
	std::string		uploadPath = resolvePath("");
	std::string		fileName;
	std::ofstream	ofs;
	char			buffer[4056];
	std::streamsize	read;

	uploadPath.append(loc.getUploadStore());
	fileName  = std::string(req.getURI(), req.getURI().find_last_of('/') + 1);
	if (fileName.empty())
		return (client.getResponse().buildErrorResponse(400, this, &loc));
	uploadPath += "/" + fileName;
	ifs.open(req.getBodyFilename().c_str(), std::ios_base::in | std::ios_base::binary);
	ofs.open(uploadPath.c_str(), std::ios_base::out | std::ios_base::binary);
	if (!ifs.is_open() || !ofs.is_open())
		return (client.getResponse().buildErrorResponse(500, this, &loc));
	while (!ifs.eof() && !ifs.bad())
	{
		ifs.read(buffer, sizeof(buffer));
		read = ifs.gcount();
		ofs.write(buffer, read);
	}
	client.getResponse().buildPostResponse(uploadPath);
}

void	Server::handlePOST(Location &loc, Client &client, std::string path, int epollFD)
{
	try
	{
		Request	&req = client.getRequest();
		//handle POST Cgi
		if (req.getURI().find(".py") != std::string::npos || req.getURI().find(".php") != std::string::npos) //.php or any other handled cgi
			return (client.getResponse().buildPostCgiResponse(client, &loc, epollFD, *this, path));
		if (loc.getUploadStore().empty())
			return (client.getResponse().buildErrorResponse(403, this, &loc));
		return (uploadFile(loc, client));
	}
	catch(const std::exception& e)
	{
		return (client.getResponse().buildErrorResponse(500, this, &loc));
	}
}


void	Server::handleDELETE(Location &loc, Client &client, std::string& path)
{
	try
	{
		return (client.getResponse().buildDelResponse(client, path, this, &loc));
	}
	catch(const std::exception& e)
	{
		std::cerr << e.what() << std::endl;
		return (client.getResponse().buildErrorResponse(500, this, &loc));
	}
	


}

//adds errorFD to epoll so it can then add the right fds if execve succeeded
void	Server::addCgiToEpoll(CGI &cgi, int epollFD) const
{
	epoll_event	ev;
	ev.events = EPOLLIN;
	ev.data.fd = cgi.getErrorFD();
	fcntl(cgi.getErrorFD(), F_SETFL, O_NONBLOCK);
	if (epoll_ctl(epollFD, EPOLL_CTL_ADD, cgi.getErrorFD(), &ev) == -1)
	{
		//TODO
	}
}

#include "Server.hpp"

std::string	resolvePath(std::string relPath);
static int	isDir(char const *path);

/*******************************************************************************
*						DISPATCHER
*******************************************************************************/

void	Server::dispatchRequest(Client &client, int epollFD)
{
	Request		&req = client.getRequest();
	Response	&resp = client.getResponse();


	//when error already built, exit straight away
	if (resp.getRespFlag())
		return;
	_getQueryParams(req);
	int	locIdx = _matchLocation(req.getURI());
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

	std::string	path = buildPath(req.getURI(), &loc);
	if (isDir(path.c_str()) || *(path.rbegin()) == '/')
		return (_handleDir(client, loc, path));
	if (!isMethodAllowed(req.getMethod(), loc))
		return (resp.build405Response(isMethodAllowed(GET, loc), isMethodAllowed(POST, loc), isMethodAllowed(DELETE, loc), this, &loc));

	if (req.getMethod() == GET)
		_handleGET(client, loc, path, epollFD);
	else if (req.getMethod() == POST)
		_handlePOST(loc, client, path, epollFD);
	else
		_handleDELETE(loc, client, path);
}


/*******************************************************************************
*						UTILS
*******************************************************************************/

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

int	Server::_matchLocation(std::string URI) const
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

std::string	Server::buildPath(std::string URI, Location *loc) const
{
	std::string	path;
	if (loc && !loc->getRoot().empty())
		path = resolvePath(loc->getRoot());
	else if (!this->_root.empty())
		path = resolvePath(this->_root);
	if (path == "")
		path = resolvePath(URI);
	else
		path.append(URI);
	return (path);
}

static int	isDir(char const *path)
{
   struct stat	statbuf;
   if (stat(path, &statbuf) != 0)
	   return (0);
   return S_ISDIR(statbuf.st_mode);
}

void	Server::_handleDir(Client &client, Location &loc, std::string dir)
{
	if (!loc.getIndex().empty()) //build location path first if it exists
		return (client.getResponse().buildRouteResponse(buildPath("/" + loc.getIndex(), &loc), this, &loc));
	else if (!this->_index.empty()) //else, build default location path for server
		return (client.getResponse().buildRouteResponse(buildPath("/" + this->_index, &loc), this, &loc));
	else if (loc.getAutoIndex() == 1 || (loc.getAutoIndex() != 0 && this->_autoIndex == 1))
		return (client.getResponse().buildDirectoryListingResponse(dir, this, &loc));
	if (*(dir.rbegin()) != '/')
		dir += "/";
	dir.append("index.html");
	std::string	path = buildPath(dir, &loc);
	if (access(path.c_str(), R_OK) == -1)
		return (client.getResponse().buildErrorResponse(403, this, &loc));
	else
		return (client.getResponse().buildRouteResponse(path, this, &loc)); 
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

#include "Server.hpp"

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

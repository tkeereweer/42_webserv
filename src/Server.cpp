#include "../include/Server.hpp"
#include <stdexcept>

/*******************************************************************************
*						CTOR/DTOR
*******************************************************************************/

Server::Server(void): _maxBodySizeClientReq(1024) {}

Server::Server(Server const &src):
	_name(src._name),
	_sockets(src._sockets),
	_maxBodySizeClientReq(src._maxBodySizeClientReq),
	_locations(src._locations),
	_root(src._root)
{}

Server	&Server::operator=(Server const &rhs)
{
	if (this != &rhs)
	{
		this->_sockets = rhs._sockets;
		this->_locations = rhs._locations;
		this->_name = rhs._name;
		this->_maxBodySizeClientReq = rhs._maxBodySizeClientReq;
		this->_root = rhs._root;
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

long long	Server::getMaxBody(void) const
{
	return (this->_maxBodySizeClientReq);
}

std::vector<Location>	&Server::getLocations(void)
{
	return (this->_locations);
}

std::string	Server::getServerRoot(void) const
{
	return (this->_root);
}

void	Server::setName(std::string name)
{
	this->_name = name;
}

void	Server::setMaxBody(long long maxBody)
{
	this->_maxBodySizeClientReq = maxBody;
}

void	Server::setServerRoot(std::string root)
{
	this->_root = root;
}

void	Server::addSocket(t_socket socket)
{
	this->_sockets.push_back(socket);
}

void	Server::addLocation(Location location)
{
	this->_locations.push_back(location);
}

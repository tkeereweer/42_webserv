#include "../include/Server.hpp"
#include <stdexcept>

/*******************************************************************************
*						CTOR/DTOR
*******************************************************************************/

Server::Server(void): _maxBodySizeClientReq(256) {}

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

std::vector<t_socket>	&Server::getSockets(void)
{
	return (this->_sockets);
}

void	Server::addSocket(t_socket socket)
{
	this->_sockets.push_back(socket);
}

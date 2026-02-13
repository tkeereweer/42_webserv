#include "../include/Server.hpp"
#include <stdexcept>

Server::Server(void):
	_numSockets(1),
	_sockets(new Socket[1]),
	_maxBodySizeClientReq(256),
	_numLocations(1),
	_locations(new Location[1])
{}

Server::Server(Server const &src):
	_name(src._name),
	_numSockets(src._numSockets),
	_sockets(new Socket[src._numSockets]),
	_maxBodySizeClientReq(src._maxBodySizeClientReq),
	_numLocations(src._numLocations),
	_locations(new Location[src._numLocations]),
	_root(src._root)
{
	for (int i = 0; i < src._numSockets; i++)
		this->_sockets[i] = src._sockets[i];
	for (int i = 0; i < src._numLocations; i++)
		this->_locations[i] = src._locations[i];
}

Server	&Server::operator=(Server const &rhs)
{
	delete[] this->_sockets;
	this->_sockets = new Socket[rhs._numSockets];
	delete[] this->_locations;
	this->_locations = new Location[rhs._numLocations];
	this->_name = rhs._name;
	this->_numSockets = rhs._numSockets;
	this->_numLocations = rhs._numLocations;
	this->_root = rhs._root;
	for (int i = 0; i < rhs._numSockets; i++)
		this->_sockets[i] = rhs._sockets[i];
	for (int i = 0; i < rhs._numLocations; i++)
		this->_locations[i] = rhs._locations[i];
	return (*this);
}

Server::~Server(void)
{
	delete[] this->_sockets;
	delete[] this->_locations;
}

int	Server::getNumSockets(void) const
{
	return (this->_numSockets);
}

Socket	Server::getSocket(int i) const
{
	if (i < this->_numSockets)
		return (this->_sockets[i]);
	throw(std::out_of_range("Socket index out of range"));
}

void	Server::setSocket(Socket socket, int pos)
{
	this->_sockets[pos] = socket;
}

#include "../include/Socket.hpp"


/*******************************************************************************
*						CTOR/DTOR
*******************************************************************************/

Socket::Socket(void) {}

Socket::Socket(std::string port, std::string ip):
	_port(port),
	_ipAddr(ip)
{}

Socket::Socket(Socket const &src):
	_port(src._port),
	_ipAddr(src._ipAddr)
{}

Socket	&Socket::operator=(Socket const &rhs)
{
	if (this != &rhs)
	{
		this->_port = rhs._port;
		this->_ipAddr = rhs._ipAddr;
	}
	return (*this);
}

Socket::~Socket(void) {}


/*******************************************************************************
*						GET
*******************************************************************************/

std::string	Socket::getPort(void) const
{
	return (this->_port);
}

std::string	Socket::getIpAddr(void) const
{
	return (this->_ipAddr);
}

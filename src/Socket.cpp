#include "../include/Socket.hpp"

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
	this->_port = rhs._port;
	this->_ipAddr = rhs._ipAddr;
	return (*this);
}

Socket::~Socket(void) {}

std::string	Socket::getPort(void) const
{
	return (this->_port);
}

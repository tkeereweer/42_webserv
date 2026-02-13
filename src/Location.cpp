#include "../include/Location.hpp"

Location::Location(void) {}

Location::Location(Location const &src):
	_path(src._path)
{
	for (unsigned int i = 0; i < 3; i++)
		this->_acceptedMethods[i] = src._acceptedMethods[i];
}

Location	&Location::operator=(Location const &rhs)
{
	this->_path = rhs._path;
	for (unsigned int i = 0; i < 3; i++)
		this->_acceptedMethods[i] = rhs._acceptedMethods[i];
	return (*this);
}

Location::~Location(void) {}

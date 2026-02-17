#include "../include/Location.hpp"

/*******************************************************************************
*						CTOR/DTOR
*******************************************************************************/

Location::Location(void):
	_acceptGET(false),
	_acceptPOST(false),
	_acceptDELETE(false)
{}

Location::Location(Location const &src):
	_path(src._path),
	_acceptGET(src._acceptGET),
	_acceptPOST(src._acceptPOST),
	_acceptDELETE(src._acceptDELETE),
	_root(src._root),
	_autoIndex(src._autoIndex),
	_index(src._index),
	_uploadStore(src._uploadStore)
	
{}

Location	&Location::operator=(Location const &rhs)
{
	if (this != &rhs)
	{
		this->_path = rhs._path;
		this->_acceptGET = rhs._acceptGET;
		this->_acceptPOST = rhs._acceptPOST;
		this->_acceptDELETE = rhs._acceptDELETE;
		this->_root = rhs._root;
		this->_autoIndex = rhs._autoIndex;
		this->_uploadStore = rhs._uploadStore;
		this->_index = rhs._index;
	}
	return (*this);
}

Location::~Location(void) {}

/*******************************************************************************
*						GET/SET
*******************************************************************************/

std::string	Location::getPath(void) const
{
	return (this->_path);
}

bool		Location::getAcceptGET(void) const
{
	return (this->_acceptGET);
}

bool		Location::getAcceptPOST(void) const
{
	return (this->_acceptPOST);
}

bool		Location::getAcceptDELETE(void) const
{
	return (this->_acceptDELETE);
}

std::string	Location::getLocRoot(void) const
{
	return (this->_root);
}

bool		Location::getAutoIndex(void) const
{
	return (this->_autoIndex);
}

std::string	Location::getIndex(void) const
{
	return (this->_index);
}

std::string	Location::getUploadStore(void) const
{
	return (this->_uploadStore);
}

void	Location::setPath(std::string path)
{
	this->_path = path;
}

void	Location::setAcceptGET(bool accept)
{
	this->_acceptGET = accept;
}

void	Location::setAcceptPOST(bool accept)
{
	this->_acceptPOST = accept;
}

void	Location::setAcceptDELETE(bool accept)
{
	this->_acceptDELETE = accept;
}

void	Location::setLocRoot(std::string root)
{
	this->_root = root;
}

void	Location::setAutoIndex(bool on)
{
	this->_autoIndex = on;
}

void	Location::setIndex(std::string file)
{
	this->_index = file;
}

void	Location::setUploadStore(std::string dir)
{
	this->_uploadStore = dir;
}

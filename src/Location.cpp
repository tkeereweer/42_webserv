#include "../include/Location.hpp"

/*******************************************************************************
*						CTOR/DTOR
*******************************************************************************/

Location::Location(void): Config() {}

Location::Location(Location const &src):
	Config(src),
	_path(src._path),
	_uploadStore(src._uploadStore)
{}

Location	&Location::operator=(Location const &rhs)
{
	if (this != &rhs)
	{
		this->_path = rhs._path;
		this->_uploadStore = rhs._uploadStore;
		this->_acceptGET = rhs._acceptGET;
		this->_acceptPOST = rhs._acceptPOST;
		this->_acceptDELETE = rhs._acceptDELETE;
		this->_root = rhs._root;
		this->_autoIndex = rhs._autoIndex;
		this->_index = rhs._index;
		this->_maxBodySizeClientReq = rhs._maxBodySizeClientReq;
		this->_errorPages = rhs._errorPages;
		this->_redirect = rhs._redirect;
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


std::string	Location::getUploadStore(void) const
{
	return (this->_uploadStore);
}

void	Location::setPath(std::string path)
{
	this->_path = path;
}

void	Location::setUploadStore(std::string dir)
{
	this->_uploadStore = dir;
}

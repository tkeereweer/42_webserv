#include "../include/Config.hpp"

/*******************************************************************************
*						CTOR/DTOR
*******************************************************************************/

Config::Config(void):
	_acceptGET(true),
	_acceptPOST(false),
	_acceptDELETE(false),
	_autoIndex(false),
	_maxBodySizeClientReq(1024)
{}

Config::Config(Config const &src):
	_acceptGET(src._acceptGET),
	_acceptPOST(src._acceptPOST),
	_acceptDELETE(src._acceptDELETE),
	_root(src._root),
	_autoIndex(src._autoIndex),
	_index(src._index),
	_maxBodySizeClientReq(src._maxBodySizeClientReq),
	_errorPages(src._errorPages),
	_redirect(src._redirect)
{}

Config	&Config::operator=(Config const &rhs)
{
	if (this != &rhs)
	{
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

Config::~Config(void) {}

/*******************************************************************************
*						GET/SET
*******************************************************************************/

bool		Config::getAcceptGET(void) const
{
	return (this->_acceptGET);
}

bool		Config::getAcceptPOST(void) const
{
	return (this->_acceptPOST);
}

bool		Config::getAcceptDELETE(void) const
{
	return (this->_acceptDELETE);
}

std::string	Config::getRoot(void) const
{
	return (this->_root);
}

bool		Config::getAutoIndex(void) const
{
	return (this->_autoIndex);
}

std::string	Config::getIndex(void) const
{
	return (this->_index);
}

long long	Config::getMaxBody(void) const
{
	return (this->_maxBodySizeClientReq);
}

std::map<int, std::string>	&Config::getErrorPages(void)
{
	return (this->_errorPages);
}

std::pair<int, std::string>	Config::getRedir(void) const
{
	return (this->_redirect);
}

void	Config::setAcceptGET(bool accept)
{
	this->_acceptGET = accept;
}

void	Config::setAcceptPOST(bool accept)
{
	this->_acceptPOST = accept;
}

void	Config::setAcceptDELETE(bool accept)
{
	this->_acceptDELETE = accept;
}

void	Config::setRoot(std::string root)
{
	this->_root = root;
}

void	Config::setAutoIndex(bool on)
{
	this->_autoIndex = on;
}

void	Config::setIndex(std::string file)
{
	this->_index = file;
}

void	Config::setMaxBody(long long maxBody)
{
	this->_maxBodySizeClientReq = maxBody;
}

void	Config::addErrorPage(int code, std::string page)
{
	this->_errorPages.insert(std::pair<int, std::string>(code, page));
}

void	Config::setRedirect(int status, std::string path)
{
	this->_redirect = std::pair<int, std::string>(status, path);
}

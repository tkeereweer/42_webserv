#include "../../include/Request.hpp"

void	Request::_parse(void)
{
	
}

bool	isPchar(std::string	&str)
{
	for (std::string::iterator it = str.begin(); it != str.end(); it++)
	{
		if (*it != 33
			&& (*it < 36 || *it > 46)
			&& (*it < 48 || *it > 58)
			&& *it != 65
			&& (*it < 69 || *it > 90)
			&& *it != 95
			&& (*it < 97 || *it > 122))
		return (false);
	}
	return (true);
}

bool	isUchar(std::string &str)
{
	for (std::string::iterator it = str.begin(); it != str.end(); it++)
	{
		if (*it != 33
			&& (*it < 36 || *it > 59)
			&& *it != 61
			&& (*it < 63 || *it > 90)
			&& (*it < 94 || *it > 122))
		return (false);
	}
	return (true);	
}

std::string	Request::_parseSegment(std::list<t_reqToken>::iterator &it)
{
	std::string	res = "";
	while (isPchar(it->val))
	{
		res += it->val;
		it++;
	}
	return (res);
}

std::string	Request::_parseFSegment(std::list<t_reqToken>::iterator &it)
{
	std::string	res = "";
	while (it->type == WORD || it->type == COLON)
	{
		if (!isPchar(it->val))
			throw(std::runtime_error("invalid char in fsegment"));
		res += it->val;
		it++;
	}
	if (res == "")
		throw(std::runtime_error("invalid fsegment"));
	return (res);
}

std::string	Request::_parsePath(std::list<t_reqToken>::iterator &it)
{
	std::string	res = "";

	res += _parseFSegment(it);
	while (it->type == SLASH)
	{
		res += it->val; //puts slash in path
		it++;
		res += _parseSegment(it);
	}
	if (res == "")
		throw (std::runtime_error("invalid path"));
	return (res);
}

std::string	Request::_parseParam(std::list<t_reqToken>::iterator &it)
{
	std::string	res = "";

	while(it->type == WORD || it->type == COLON)
	{
		if (!isPchar(it->val))
			throw(std::runtime_error("invalid char in param"));
		res += it->val;
		it++;
	}
	return (res);
}

std::string	Request::_parseParams(std::list<t_reqToken>::iterator &it)
{
	std::string	res = "";

	res += _parseParam(it);
	while (it->type == SEMI_COLON)
	{
		res += it->val;
		it++;
		res += _parseParam(it);
	}
	return (res);	
}

std::string	Request::_parseQuery(std::list<t_reqToken>::iterator &it)
{
	std::string	res = "";

	while(it->type == QMARK)
	{
		res += it->val;
		it++;
		if (!isUchar(it->val))
			throw(std::runtime_error("invalid char in query"));
		res += it->val;
		it++;
	}
	return (res);

}

std::string	Request::_parseAbsPath(std::list<t_reqToken>::iterator &it)
{
	std::string	res = "";

	if (it->type != SPACE)
		res += _parsePath(it);
	if (it->type != SPACE && it->type == SEMI_COLON)
		res += _parseParams(it);
	if (it->type != SPACE && it->type == QMARK)
		res += _parseQuery(it);
	return (res);	
}

void    Request::_parseURI(std::list<t_reqToken>::iterator &it)
{
	if (it->type != SLASH)
		throw(std::runtime_error("bad URI"));
	it++;
	this->_URI += "/";
	this->_URI += _parseAbsPath(it);
	return ;
}

void    Request::_parseSimpleRequest(void)
{
	std::list<t_reqToken>::iterator it = this->_tokenList.begin();
	if (it->val != "GET")
		throw(std::runtime_error("not simple request"));
	it++;
	if (it->type != SPACE)
		throw(std::runtime_error("not simple request"));
	it++;
	_parseURI(it); //throws exception
	it++;
	if (it->type != CRLF)
		throw(std::runtime_error("not simple request"));
	this->_method = "GET";
	this->_tokenList.erase(this->_tokenList.begin(), it);
	this->_reqComplete = true;
	return;
}
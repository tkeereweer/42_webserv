#include "Request.hpp"


//error 414 is sent for URIs > 2k-8k depending on servers.
//set to 250 to succeed parsing test
void    Request::_parseURI(std::list<t_reqToken>::iterator &it)
{
	if (it->type != SLASH)
		throw (Request::ErrorNum("URI: doesn't start with SLASH", 400));
	it++;
	this->_URI += "/";
	this->_URI += _parseAbsPath(it);
	if (this->_URI.size() > 500)
		throw (Request::ErrorNum("URI too long", 414));
	return ;
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


bool	isPchar(std::string	&str)
{
	for (std::string::iterator it = str.begin(); it != str.end(); it++)
	{
		if (*it != 33
			&& (*it < 36 || *it > 46)
			&& (*it < 48 || *it > 58)
			&& (*it < 65 || *it > 90)
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
			throw(Request::ErrorNum("invalid char in fsegment", 400));
		res += it->val;
		it++;
	}
	if (res == "")
		throw(Request::ErrorNum("invalid fsegment", 400));
	return (res);
}

std::string	Request::_parsePath(std::list<t_reqToken>::iterator &it)
{
	std::string	res = "";

    if (it->val == "..")
    {
        it++;
        if (it->type == SLASH)
            throw (Request::ErrorNum("blocked path for security reasons", 400));
        it--;
    }
	res += _parseFSegment(it);
	while (it->type == SLASH)
	{
		res += it->val; //puts slash in path
		it++;
        if (it->val == "..")
        {
            it++;
            if (it->type == SLASH)
                throw (Request::ErrorNum("blocked path for security reasons", 400));
            it--;
        }
		res += _parseSegment(it);
	}
	if (res == "")
		throw (Request::ErrorNum("path: empty or invalid char in path", 400));
	return (res);
}

std::string	Request::_parseParam(std::list<t_reqToken>::iterator &it)
{
	std::string	res = "";

	while(it->type == WORD || it->type == COLON)
	{
		if (!isPchar(it->val))
			throw(Request::ErrorNum("invalid char in param", 400));
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

bool    validQuerySequence(std::string &str)
{
    if (*str.begin() == '=' || *str.rbegin() == '=')
        return (false);
    for (std::string::iterator it = str.begin(); it != str.end(); it++)
    {
        if (*it == '&' && *(it + 1) == '&')
            return (false);
        if (*it == '%')
        {
            std::string encoded(it + 1, it + 3);
            if (encoded.find_first_not_of("0123456789ABCDEF") != std::string::npos)
                throw (Request::ErrorNum("Invalid encoding in query string", 400));
            std::stringstream sstr(encoded);
            int hexToInt;
            sstr >> std::hex >> hexToInt;
            if (hexToInt < 32 || hexToInt > 126)
                throw (Request::ErrorNum("Invalid encoded range in query string", 400));
        }
    }
    return (true);
}

std::string	Request::_parseQuery(std::list<t_reqToken>::iterator &it)
{
	std::string	res = "";

	while(it->type == QMARK)
	{
		res += it->val;
		it++;
        if (it->type != WORD)
            throw(Request::ErrorNum("invalid char in query", 400));
		if (!isUchar(it->val))
			throw(Request::ErrorNum("invalid char in query", 400));
        if (!validQuerySequence(it->val))
            throw(Request::ErrorNum("incorrect query string", 400));
		res += it->val;
		it++;
	}
	return (res);

}
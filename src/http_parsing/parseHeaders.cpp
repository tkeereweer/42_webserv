#include "Request.hpp"

bool	isToken(std::string &str)
{
	for (std::string::iterator it = str.begin(); it != str.end(); it++)
	{
		if ((*it >= 0 && *it <= 31)
			|| *it == 127
			|| isspace(*it)
			|| *it == 34
			|| (*it >= 40 && *it <= 41)
			|| *it == 44
			|| *it == 47
			|| (*it >= 58 && *it <= 64)
			|| (*it >= 91 && *it <= 93)
			|| *it == 123
			|| *it == 125)
			return (false);
	}
	return (true);
}

bool	isTokenOrQuoted(std::string &str)
{
	bool inQuotes = false;
	for (std::string::iterator it = str.begin(); it != str.end(); it++)
	{
		if (*it == '"' && !inQuotes && it != str.begin()) //first quote not first char of part
			return (false);
		if (*it == '"')
		{
			if (inQuotes) //quote found before end of string
				return (false);
			inQuotes = true;
		}
		if ((*it >= 0 && *it <= 31)
			|| *it == 127
			|| isspace(*it)
			|| *it == 34
			|| (*it >= 40 && *it <= 41)
			|| *it == 44
			|| *it == 47
			|| (*it >= 58 && *it <= 64)
			|| (*it >= 91 && *it <= 93)
			|| *it == 123
			|| *it == 125)
			return (false);
	}
	return (true);
}

//throws exception
std::string	Request::_parseContentCoding(std::list<t_reqToken>::iterator &it)
{
	std::string	res = "";
	std::transform(it->val.begin(), it->val.end(), it->val.begin(), tolower);
	if (it->val == "x-gzip" || it->val == "x-compress")
	{
		res += it->val;
		it++;
		return (res);
	}
	if (!isToken(it->val))
		throw (Request::ErrorNum("wrong char in content-encoding", 400));
	res += it->val;
	it++;
	return (res);
}

//last one wins
bool	Request::_parseContentEncoding(std::list<t_reqToken>::iterator &it)
{
	std::string res = "";

	std::transform(it->val.begin(), it->val.end(), it->val.begin(), tolower);
	if (it->val != "content-encoding")
		return (false);
	it++;
	if (it->type != COLON)
		throw(Request::ErrorNum("invalid header: content-encoding", 400));
	it++;
	while (it->type == SPACE)
		it++;
	res += _parseContentCoding(it);
	if (it->type != CRLF)
		throw(Request::ErrorNum("invalid header: content-encoding", 400));
	this->_contentEncoding = res;
	return (true);
}

//bad request if multiple headers
bool	Request::_parseContentLength(std::list<t_reqToken>::iterator &it)
{
	std::string res = "";

	std::transform(it->val.begin(), it->val.end(), it->val.begin(), tolower);
	if (it->val != "content-length")
		return (false);
	it++;
	if (it->type != COLON)
		throw(Request::ErrorNum("invalid header: content-length", 400));
	it++;
	while (it->type == SPACE)
		it++;
	if (it->type != WORD || it->val.size() == 0)
		throw(Request::ErrorNum("invalid header: content-length", 400));
	for (std::string::iterator ite = it->val.begin(); ite != it->val.end(); ite++)
	{
		if (!isdigit(*ite))
			throw(Request::ErrorNum("invalid header: content-length", 400));
	}
	res += it->val;
	it++;
	if (it->type != CRLF)
		throw(Request::ErrorNum("invalid header: content-length", 400));
	if (this->_contentLength != 0)
		throw(Request::ErrorNum("more than 1 content-length header !", 400));
	this->_contentLength = atoll(res.c_str());
	if (this->_contentLength < 1)
		throw (Request::ErrorNum("content-lenght < 1", 400));
	return (true);
}

bool	isParameter(std::string &str)
{
	std::string::iterator it = str.begin();
	if (str.find('=') == std::string::npos)
		throw (Request::ErrorNum("missing '=' in content-type parameter", 400));
	advance(it, str.find('=', 0));
	std::string firstHalf(str.begin(), it);
	it++;
	std::string	secondHalf(it, str.end());
	if (firstHalf.size() == 0 || secondHalf.size() == 0
		|| firstHalf.find('=', 0) != std::string::npos
		|| secondHalf.find('=', 0) != std::string::npos
		|| !isToken(firstHalf)
		|| !isTokenOrQuoted(secondHalf))
		return (false);
	return (true);
}

std::string	Request::_parseMediaType(std::list<t_reqToken>::iterator &it)
{
	std::string	res = "";

	if (!isToken(it->val))
		throw (Request::ErrorNum("wrong char in media type", 400));
	res += it->val;
	it++;
	if (it->type != SLASH)
		throw (Request::ErrorNum("wrong media type format, expected '/'", 400));
	res += it->val;
	it++;
	if (!isToken(it->val))
			throw (Request::ErrorNum("expected valid parameter", 400));
	res += it->val;
	it++;
	while (it->type == SEMI_COLON)
	{
		res += "; ";
		it++;
		while (it->type == SPACE)
			it++;
		if (!isParameter(it->val))
			throw (Request::ErrorNum("expected valid parameter", 400));
		res += it->val;
		it++;
	}
	return (res);
}

//last one wins
bool	Request::_parseContentType(std::list<t_reqToken>::iterator &it)
{
	std::string res = "";

	std::transform(it->val.begin(), it->val.end(), it->val.begin(), tolower);
	if (it->val != "content-type")
		return (false);
	it++;
	if (it->type != COLON)
		throw(Request::ErrorNum("invalid header: content-type", 400));
	it++;
	while (it->type == SPACE)
		it++;
	res += _parseMediaType(it);
	if (it->type != CRLF)
		throw(Request::ErrorNum("invalid header: content-type", 400));
	this->_contentType = res;
	return (true);
}

bool	isCookie(std::string &str)
{
	std::string::iterator it = str.begin();
	if (str.find('=') == std::string::npos)
		throw (Request::ErrorNum("missing = in cookie value", 400));
	advance(it, str.find('=', 0));
	std::string firstHalf(str.begin(), it);
	it++;
	std::string	secondHalf(it, str.end());
	if (firstHalf.size() == 0 || secondHalf.size() == 0
		|| firstHalf.find('=', 0) != std::string::npos
		|| secondHalf.find('=', 0) != std::string::npos
		|| !isToken(firstHalf)
		|| !isToken(secondHalf))
		return (false);
	return (true);
}


//concatenate and separate with ';'
//cookie = token "=" token *(";" token "=" token)
bool	Request::_parseCookies(std::list<t_reqToken>::iterator &it)
{
	std::string res = "";

	std::transform(it->val.begin(), it->val.end(), it->val.begin(), tolower);
	if (it->val != "cookie")
		return (false);
	it++;
	if (it->type != COLON)
		throw(Request::ErrorNum("invalid header: cookies", 400));
	it++;
	if (it->type != SPACE)
		throw(Request::ErrorNum("invalid header: cookies", 400));
	while (it->type == SPACE)
		it++;
	if (!isCookie(it->val))
		throw(Request::ErrorNum("invalid header: cookies", 400));
	res += it->val;
	it++;
	while (it->type == SEMI_COLON)
	{
		it++;
		if (!isCookie(it->val))
			throw(Request::ErrorNum("invalid header: cookies", 400));
		res += it->val;
		it++;
	}
	if (this->_cookies != "")
		this->_cookies += "; ";
	this->_cookies += res;
	return (true);
}	

bool	isValidForHeaders(std::string &str)
{
	for (std::string::iterator it = str.begin(); it != str.end(); it++)
	{
		if (!isascii(*it)
			|| (*it >= 0 && *it < 32)
			|| *it > 126)
			throw(Request::ErrorNum("invalid char in some header body", 400));
	}
	return (true);
}

bool	isValidForHeaderName(std::string str)
{
	//transform to lower case, split on "-" and check that only ascii alphanum 
	std::transform(str.begin(), str.end(), str.begin(), tolower);

	if (str[0] == '-' || *str.rbegin() == '-')
		throw (Request::ErrorNum("invalid char in header name", 400));
	for (std::string::iterator it = str.begin(); it != str.end(); it++)
	{
		if (*it != '-' && !isdigit(*it) && !isalpha(*it))
			throw(Request::ErrorNum("invalid char in header name", 400));
	}
	return (true);
}

bool	isHeader(std::list<t_reqToken>::iterator &it)
{
	if (it->type != WORD || !isValidForHeaderName(it->val))
		throw (std::runtime_error("unexpected token in random header"));//throw here !!!
	it++;
	if (it->type != COLON)
		throw (std::runtime_error("unexpected token in random header"));
	it++;
	while (it->type == SPACE)
		it++;
	while (it->type != CRLF && isValidForHeaders(it->val)) //type check before function important
		it++;
	return (true);
}
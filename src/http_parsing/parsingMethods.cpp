#include "../../include/Request.hpp"

void	Request::_parse(void)
{
	std::list<t_reqToken>::iterator it = this->_tokenList.begin();
	try
	{
		_parseSimpleRequest();
	}
	catch(const std::exception& e)
	{
		this->_URI.clear(); //in case simple request fails after parseURI completed
		try
		{
			_parseFullRequest(it);
		}
		catch(const std::exception& e)
		{
			std::string errorMsg = "BAD REQUEST: ";
			errorMsg += e.what();
			if (it != this->_tokenList.end())
				throw (std::runtime_error(errorMsg.c_str()));
			return ;
		}
		return ;		
	}
	return ;
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
		throw (std::runtime_error("path: empty or invalid char in path"));
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
		throw(std::runtime_error("URI: doesn't start with SLASH"));
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
	_parseURI(it);
	if (it->type != CRLF)
		throw(std::runtime_error("not simple request"));
	this->_method = GET;
	this->_tokenList.erase(this->_tokenList.begin(), it);
	this->_reqComplete = true;
	return;
}

void	verifyHTTPWord(std::string str)
{
	std::string::iterator it = str.begin();
	advance(it, str.find('.', 0));
	std::string firstHalf(str.begin(), it);
	it++;
	std::string	secondHalf(it, str.end());

	if (firstHalf.size() == 0 || secondHalf.size() == 0)
		throw (std::runtime_error("wrong HTTP version"));
	if (firstHalf.find_first_not_of("0123456789") != std::string::npos
		|| secondHalf.find_first_not_of("0123456789") != std::string::npos)
		throw (std::runtime_error("one or more char not digit in HTTP version num"));
	return;
}

void	Request::_parseHTTPVersion(std::list<t_reqToken>::iterator &it)
{
	std::string	res = "";
	if (it->val == "undefined")
	{
		res += it->val;
		it++;
		this->_HTTPVersion = res;
		return;
	}
	if (it->val != "HTTP")
		throw (std::runtime_error("wrong HTTP version"));
	res += it->val;
	it++;
	if (it->type != SLASH)
		throw (std::runtime_error("wrong HTTP version"));
	res += it->val;
	it++;
	if (it->type != WORD)
		throw (std::runtime_error("wrong HTTP version"));
	verifyHTTPWord(it->val); //throws exception
	res += it->val;
	it++;
	this->_HTTPVersion = res;
	return ;
}

void		Request::_parseRequestLine(std::list<t_reqToken>::iterator &it)
{
	std::string	method;
	if (it->val != "GET" && it->val != "POST" && it->val != "DELETE")
		throw(std::runtime_error("wrong method"));
	method = it->val;
	it++;
	if (it->type != SPACE)
		throw(std::runtime_error("request line: no space after method"));
	it++;
	_parseURI(it); //throws exception
	if (it->type != SPACE)
		throw(std::runtime_error("not full request"));
	it++;
	_parseHTTPVersion(it); //throws exception
	if (it->type != CRLF)
		throw(std::runtime_error("full request not CRLF terminated"));
	it++;
	if (method == "GET")
		this->_method = GET;
	else if (method == "POST")
		this->_method = POST;
	else if (method == "DELETE")
		this->_method = DELETE;
	this->_tokenList.erase(this->_tokenList.begin(), it);
	this->_reqLineValid = true;
	return ;
}



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
		throw (std::runtime_error("wrong char in content-encoding"));
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
		throw(std::runtime_error("invalid header: content-encoding"));
	it++;
	// if (it->type != SPACE)
	// 	throw(std::runtime_error("invalid header: content-encoding")); //whitespace optional in header ?
	while (it->type == SPACE)
		it++;
	res += _parseContentCoding(it);
	if (it->type != CRLF)
		throw(std::runtime_error("invalid header: content-encoding"));
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
		throw(std::runtime_error("invalid header: content-length"));
	it++;
	// if (it->type != SPACE)
	// 	throw(std::runtime_error("invalid header: content-length"));
	while (it->type == SPACE)
		it++;
	if (it->type != WORD || it->val.size() == 0)
		throw(std::runtime_error("invalid header: content-length"));
	for (std::string::iterator ite = it->val.begin(); ite != it->val.end(); ite++)
	{
		if (!isdigit(*ite))
			throw(std::runtime_error("invalid header: content-length"));
	}
	res += it->val;
	it++;
	if (it->type != CRLF)
		throw(std::runtime_error("invalid header: content-length"));
	if (this->_contentLength != 0)
		throw(std::runtime_error("more than 1 content-length header !"));
	this->_contentLength = atoll(res.c_str());
	return (true);
}

bool	isParameter(std::string &str)
{
	std::string::iterator it = str.begin();
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
		throw (std::runtime_error("wrong char in media type"));
	res += it->val;
	it++;
	if (it->type != SLASH)
		throw (std::runtime_error("wrong media type format, expected '/'"));
	res += it->val;
	it++;
	if (!isToken(it->val))
			throw (std::runtime_error("expected valid parameter"));
	res += it->val;
	it++;
	while (it->type == SEMI_COLON)
	{
		res += "; ";
		it++;
		while (it->type == SPACE)
			it++;
		if (!isParameter(it->val))
			throw (std::runtime_error("expected valid parameter"));
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
		throw(std::runtime_error("invalid header: content-type"));
	it++;
	// if (it->type != SPACE)
	// 	throw(std::runtime_error("invalid header: content-type"));
	while (it->type == SPACE)
		it++;
	res += _parseMediaType(it);
	if (it->type != CRLF)
		throw(std::runtime_error("invalid header: content-type"));
	this->_contentType = res;
	return (true);
}

bool	isCookie(std::string &str)
{
	std::string::iterator it = str.begin();
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
		throw(std::runtime_error("invalid header: cookies"));
	it++;
	if (it->type != SPACE)
		throw(std::runtime_error("invalid header: cookies"));
	while (it->type == SPACE)
		it++;
	if (!isCookie(it->val))
		throw(std::runtime_error("invalid header: cookies"));
	res += it->val;
	it++;
	while (it->type == SEMI_COLON)
	{
		it++;
		if (!isCookie(it->val))
			throw(std::runtime_error("invalid header: cookies"));
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
		if ((*it >= 0 && *it < 32)
			|| *it > 126)
			throw(std::runtime_error("invalid char in some header body"));
	}
	return (true);
}

static bool	isHeader(std::list<t_reqToken>::iterator &it)
{
	if (it->type != WORD)
		return (false);
	it++;
	if (it->type != COLON)
		return (false);
	it++;
	while (it->type == SPACE)
		it++;
	while (it->type != CRLF && isValidForHeaders(it->val)) //type check before function important
		it++;
	return (true);
}

void	Request::_parseFullRequest(std::list<t_reqToken>::iterator &it)
{
	//throws exception when request line not full
	//empties token list
	if (!this->_reqLineValid)
		_parseRequestLine(it);
	while (it->type != CRLF && it != this->_tokenList.end())
	{
		//these functions must advance it beyond next CRLFwhen succesful
		//throw exception if header format not respected --> in body overshoots in bad requests handled
		if (!(_parseContentEncoding(it)
			|| _parseContentLength(it)
			|| _parseContentType(it)
			|| _parseCookies(it)))
		{
			try
			{
				isHeader(it);// makes a copy
			}
			catch(const std::exception& e)
			{
				throw(std::runtime_error("not header in header part"));
			}
			if (it == this->_tokenList.end())
				break;
			while (it->type != CRLF)
				it++;
		}
		it++;
		this->_tokenList.erase(this->_tokenList.begin(), it);
	}
	if (it == this->_tokenList.end()) //if we get to end of list w/o 2xCRLF
	{
		while (it->type != CRLF && it != this->_tokenList.begin())
			it--;
		it++;
		this->_tokenList.erase(this->_tokenList.begin(), it);
		//end up here with tokenlist containing non parsed content
		//will be parsed next call
		return ;
	}
	it++;
	this->_reqHeadersValid = true;
	if (it != this->_tokenList.end() && this->_method == POST && this->_contentLength > 0)
		_readLeftovers(it);
	else
		this->_reqComplete = true;
	return ;
}

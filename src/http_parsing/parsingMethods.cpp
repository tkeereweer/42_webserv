#include "Request.hpp"

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
			if (it != this->_tokenList.end())
			{
				// std::cerr << e.what() << std::endl;
				throw; //this might do something called slicing ? do std::cerr<< e.what(); throw; instead ?
			}
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

void    Request::_parseSimpleRequest(void)
{
	std::list<t_reqToken>::iterator it = this->_tokenList.begin();
	if (it->val != "GET")
		throw(Request::Error405("not simple request"));
	it++;
	if (it->type != SPACE)
		throw(Request::ErrorNum("not simple request", 400));
	it++;
	_parseURI(it);
	if (it->type != CRLF)
		throw(Request::ErrorNum("not simple request", 400));
	it++;
	if (it != this->_tokenList.end())
		throw(Request::ErrorNum("not simple request", 400));
	this->_method = GET;
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
		throw (Request::ErrorNum("wrong HTTP version", 400));
	if (firstHalf.find_first_not_of("0123456789") != std::string::npos
		|| secondHalf.find_first_not_of("0123456789") != std::string::npos)
		throw (Request::ErrorNum("one or more char not digit in HTTP version num", 400));
	if (firstHalf != "1" && firstHalf != "2")
		throw (Request::ErrorNum("unsupported HTTP version", 505));
	if (secondHalf != "0" && secondHalf != "1")
		throw (Request::ErrorNum("unsupported HTTP version", 505));
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
		throw (Request::ErrorNum("wrong HTTP version", 400));
	res += it->val;
	it++;
	if (it->type != SLASH)
		throw (Request::ErrorNum("wrong HTTP version", 400));
	res += it->val;
	it++;
	if (it->type != WORD)
		throw (Request::ErrorNum("wrong HTTP version", 400));
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
	{
		if (it->val.find_first_not_of("ABCDEFGHIJKLMNOPQRSTUVWXYZ") != std::string::npos)
			throw(Request::ErrorNum("request line: invalid syntax for method", 400));
		throw(Request::Error405("wrong method"));
	}
	method = it->val;
	it++;
	if (it->type != SPACE)
		throw(Request::ErrorNum("request line: no space after method", 400));
	it++;
	_parseURI(it); //throws exception
	if (it->type != SPACE)
		throw(Request::ErrorNum("not full request", 400));
	it++;
	_parseHTTPVersion(it); //throws exception
	if (it->type != CRLF)
		throw(Request::ErrorNum("full request not CRLF terminated", 400));
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

static bool	isHeader(std::list<t_reqToken>::iterator &it)
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
				throw(Request::ErrorNum("not header in header part", 400));
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
	if (it != this->_tokenList.end() && this->_method == POST)
	{
        if (this->_contentLength == 0)
            throw (Request::ErrorNum("content lenght required", 411));
		if (this->_contentType.empty())
			throw (Request::ErrorNum("invalid POST request: missing content-length or content-type", 400));
		_readLeftovers(it);
	}
	else
		this->_reqComplete = true;
	return ;
}

#include "CGI.hpp"

static bool	isHeader(std::list<t_cgiToken>::iterator &it)
{
	if (it->type != CGI_WORD)
		return (false);
	it++;
	if (it->type != CGI_COLON)
		return (false);
	it++;
	while (it->type == CGI_SPACE)
		it++;
	while (it->type != CGI_LB && isValidForHeaders(it->val)) //type check before function important
		it++;
	return (true);
}

//bad request if multiple headers
bool	CGI::_parseContentLength(std::list<t_cgiToken>::iterator &it)
{
	std::string res = "";

	std::transform(it->val.begin(), it->val.end(), it->val.begin(), tolower);
	if (it->val != "content-length")
		return (false);
	it++;
	if (it->type != CGI_COLON)
		throw(std::runtime_error("invalid header: content-length"));
	it++;
	if (it->type != CGI_SPACE)
		throw(std::runtime_error("invalid header: content-length"));
	while (it->type == CGI_SPACE)
		it++;
	if (it->type != CGI_WORD || it->val.size() == 0)
		throw(std::runtime_error("invalid header: content-length"));
	for (std::string::iterator ite = it->val.begin(); ite != it->val.end(); ite++)
	{
		if (!isdigit(*ite))
			throw(std::runtime_error("invalid header: content-length"));
	}
	res += it->val;
	it++;
	if (it->type != CGI_LB)
		throw(std::runtime_error("invalid header: content-length"));
	if (this->_contentLength != -1)
		throw(std::runtime_error("more than 1 content-length header !"));
	this->_contentLength = atoll(res.c_str());
	return (true);
}

std::string	CGI::_parseMediaType(std::list<t_cgiToken>::iterator &it)
{
	std::string	res = "";

	// if (!isToken(it->val))
	// 	throw (std::runtime_error("wrong char in media type"));
	res += it->val;
	it++;
	if (it->type != CGI_SLASH)
		throw (std::runtime_error("wrong media type format, expected '/'"));
	res += it->val;
	it++;
	if (!isToken(it->val))
			throw (std::runtime_error("expected valid parameter"));
	res += it->val;
	it++;
	while (it->type == CGI_SEMI_COLON)
	{
		it++;
		if (!isToken(it->val))
			throw (std::runtime_error("expected valid parameter"));
		res += it->val;
		it++;
	}
	return (res);
}

//last one wins
bool	CGI::_parseContentType(std::list<t_cgiToken>::iterator &it)
{
	std::string res = "";

	std::transform(it->val.begin(), it->val.end(), it->val.begin(), tolower);
	if (it->val != "content-type")
		return (false);
	it++;
	if (it->type != CGI_COLON)
		throw(std::runtime_error("invalid header: content-type"));
	it++;
	if (it->type != CGI_SPACE)
		throw(std::runtime_error("invalid header: content-type"));
	while (it->type == CGI_SPACE)
		it++;
	res += _parseMediaType(it);
	if (it->type != CGI_LB)
		throw(std::runtime_error("invalid header: content-type"));
	this->_contentType = res;
	return (true);
}

static bool	isHandledStatus(int code)
{
	if (code == 200 || code == 202 || code == 204
		|| code == 300 || code == 301 || code == 302
		|| code == 400 || code == 403 || code == 404 || code == 405 || code == 408 || code == 411 || code == 413
		|| code == 500 || code == 502 || code == 503)
		return (true);
	return (false);
}

bool	CGI::_parseStatus(std::list<t_cgiToken>::iterator &it)
{
	std::string res = "";
	int			status;

	std::transform(it->val.begin(), it->val.end(), it->val.begin(), tolower);
	if (it->val != "status")
		return (false);
	it++;
	if (it->type != CGI_COLON)
		throw(std::runtime_error("invalid header: status"));
	it++;
	if (it->type != CGI_SPACE)
		throw(std::runtime_error("invalid header: status"));
	while (it->type == CGI_SPACE)
		it++;
	if (it->type != CGI_WORD || it->val.size() == 0)
		throw(std::runtime_error("invalid header: status"));
	for (std::string::iterator ite = it->val.begin(); ite != it->val.end(); ite++)
	{
		if (!isdigit(*ite))
			throw(std::runtime_error("invalid header: status"));
	}
	res += it->val;
	it++;
	if (it->type != CGI_LB)
		throw(std::runtime_error("invalid header: status"));
	status = atoi(res.c_str());
	if (!isHandledStatus(status))
		return (false);
	this->_status = atoi(res.c_str());
	return (true);
}

//this function transfers as-is the cookie header
bool	CGI::_parseSetCookies(std::list<t_cgiToken>::iterator &it)
{
	std::string res = "";

	std::transform(it->val.begin(), it->val.end(), it->val.begin(), tolower);
	if (it->val != "set-cookie")
		return (false);
	it++;
	if (it->type != CGI_COLON)
		throw(std::runtime_error("invalid header: Set-Cookie"));
	it++;
	while (it->type == CGI_SPACE)
		it++;
	while (it->type != CGI_LB)
	{
		res += it->val;
		it++;
	}
	this->_setCookie = res;
	return (true);
}

bool	CGI::_parseLocationHeader(std::list<t_cgiToken>::iterator &it)
{
	std::string res = "";

	std::transform(it->val.begin(), it->val.end(), it->val.begin(), tolower);
	if (it->val != "Location")
		return (false);
	it++;
	if (it->type != CGI_COLON)
		throw(std::runtime_error("invalid header: Location"));
	it++;
	while (it->type == CGI_SPACE)
		it++;
	while (it->type != CGI_LB)
	{
		res += it->val;
		it++;
	}
	this->_locationHeader = res;
	return (true);
}

void	CGI::_readLeftovers(std::list<t_cgiToken>::iterator &it)
{
	this->_outBuff.clear();
	for (std::list<t_cgiToken>::iterator ite = it; ite != this->_CGItokenList.end(); ite++)
		this->_outBuff.append(ite->val.begin(), ite->val.end());
	return ;
}

void	CGI::_parseCGIOutput(std::list<t_cgiToken>::iterator &it)
{
	while (it->type != CGI_LB && it != this->_CGItokenList.end())
	{
		//these functions must advance it beyond next CRLFwhen succesful
		//throw exception if header format not respected --> in body overshoots in bad requests handled
		if (!(_parseContentLength(it)
			|| _parseContentType(it)
			|| _parseStatus(it)
			|| _parseSetCookies(it)
			|| _parseLocationHeader(it))) //_parseSetCookie as-is so no grammar checking !
		{
			try
			{
				isHeader(it);// makes a copy
			}
			catch(const std::exception& e)
			{
				throw(std::runtime_error("not header in header part"));
			}
			if (it == this->_CGItokenList.end())
				break;
			while (it->type != CGI_LB)
				it++;
		}
		it++;
		this->_CGItokenList.erase(this->_CGItokenList.begin(), it);
	}
	if (it == this->_CGItokenList.end()) //if we get to end of list w/o 2xCRLF
	{
		while (it->type != CGI_LB && it != this->_CGItokenList.begin())
			it--;
		it++;
		this->_CGItokenList.erase(this->_CGItokenList.begin(), it);
		//end up here with tokenlist containing non parsed content
		//will be parsed next call
		return ;
	}
	it++;
	this->_outHeadersValid = true;
	if (it != this->_CGItokenList.end())
		_readLeftovers(it);
	else
	{
		this->_outBuff.clear();
		this->_outComplete = true;
	}
	return ;
}

//Returns -1 if request complete, 0 if expecting more headers, # of bytes to read until end of header.
//Don't call again if -1 or >0
//Consumes data such that data=="leftover after CRLF" after each call.
int	CGI::lexCGIOutput(std::string &data)
{
	if (data == "")
		throw(std::runtime_error("empty data field"));

	this->_lexInput(data);

	std::list<t_cgiToken>::reverse_iterator	rit = this->_CGItokenList.rbegin();
	while (rit->type != CGI_LB && rit != this->_CGItokenList.rend())
		rit++;
	if (rit == this->_CGItokenList.rend())
		return (this->_CGItokenList.clear(), 0);
	//throw exception only if parsing interupted on bad grammar
	try
	{
		std::list<t_cgiToken>::iterator	it = this->_CGItokenList.begin();
		_parseCGIOutput(it);
	}
	catch(std::exception const &e)
	{
		throw(std::runtime_error(e.what()));
	}

	if (this->_outComplete)
		return (this->_CGItokenList.clear(), -1);
	// data.clear(); //issue here where we have deleted token list and we then clear data, losing info
	this->_CGItokenList.clear();
	if (!(this->_outHeadersValid))
		return (0);
	else
		return (this->_CGItokenList.clear(), 1);
}

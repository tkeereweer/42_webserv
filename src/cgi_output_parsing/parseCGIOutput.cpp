#include "../include/CGI.hpp"

static bool	isHeader(std::list<t_cgiToken>::iterator &it)
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

//bad request if multiple headers
bool	CGI::_parseContentLength(std::list<t_cgiToken>::iterator &it)
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
	if (it->type != CGI_LB)
		throw(std::runtime_error("invalid header: content-length"));
	if (this->_contentLength != 0)
		throw(std::runtime_error("more than 1 content-length header !"));
	this->_contentLength = atoll(res.c_str());
	return (true);
}

std::string	CGI::_parseMediaType(std::list<t_cgiToken>::iterator &it)
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
	if (it->type != COLON)
		throw(std::runtime_error("invalid header: content-type"));
	it++;
	// if (it->type != SPACE)
	// 	throw(std::runtime_error("invalid header: content-type"));
	while (it->type == SPACE)
		it++;
	res += _parseMediaType(it);
	if (it->type != CGI_LB)
		throw(std::runtime_error("invalid header: content-type"));
	this->_contentType = res;
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
			|| _parseContentType(it)))
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
		this->_outComplete = true;
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
		return (0);
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
		return (-1);
	data.clear(); //issue here where we have deleted token list and we then clear data, losing info

	//pop back in data last potentially unread token then pop_back()
	if (!this->_CGItokenList.empty() && (this->_CGItokenList.back().type == WORD || this->_CGItokenList.back().type == SPACE))
	{
		data.append(this->_CGItokenList.back().val.begin(), this->_CGItokenList.back().val.end());
		this->_CGItokenList.pop_back();
	}

	if (!(this->_outHeadersValid))
		return (0);
	else
		return (this->_contentLength - this->_bytesRead);
}

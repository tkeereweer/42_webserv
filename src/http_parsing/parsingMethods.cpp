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

bool	isHeader(std::list<t_reqToken>::iterator &it);

//throws exception when request line not full
//empties token list
void	Request::_parseFullRequest(std::list<t_reqToken>::iterator &it)
{
	if (!this->_reqLineValid)
		_parseRequestLine(it);
	while (it->type != CRLF && it != this->_tokenList.end())
	{
		//these functions must advance it beyond next CRLFwhen succesful
		if (!(_parseContentEncoding(it) || _parseContentLength(it) || _parseContentType(it) || _parseCookies(it)))
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
		this->_tokenList.erase(this->_tokenList.begin(), it); //end up here with tokenlist containing non parsed content, will be parsed next call		
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

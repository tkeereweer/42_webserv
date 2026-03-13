#include "Request.hpp"


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
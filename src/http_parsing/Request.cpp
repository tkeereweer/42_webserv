#include "../../include/Request.hpp"

Request::Request(void): _reqComplete(false){}

Request::Request(Request const &src)
{
	*this = src;
}

Request	&Request::operator=(Request const &rhs)
{
	if (this != &rhs)
	{
		this->_method = rhs._method;
		this->_URI = rhs._URI;
		this->_HTTPVersion = rhs._HTTPVersion;
		this->_date = rhs._date;
		this->_contentEncoding = rhs._contentEncoding;
		this->_contentLength = rhs._contentLength;
		this->_contentType = rhs._contentType;
		this->_cookies = rhs._cookies;
		this->_bodyFilename = rhs._bodyFilename;
	}
	return (*this);
}

Request::~Request(void)
{
	if (this->_bodyFilename != "")
		unlink(this->_bodyFilename.c_str());
}

long long		Request::_requestEval(std::string &data)
{
	std::list<t_reqToken>::const_reverse_iterator it = this->_tokenList.rbegin();

	while (it->type != CRLF && it != this->_tokenList.rend())
		it++;

	//no CRLF found
	//insert last token that could have been cut short
	if (it == this->_tokenList.rend() && (this->_tokenList.rbegin()->type == WORD && this->_tokenList.rbegin()->type == SPACE))
	{
		data.insert(data.begin(), this->_tokenList.rbegin()->val.begin(), this->_tokenList.rbegin()->val.end());
		this->_tokenList.pop_back();
		return (0);
	}

	//found a CRLF, checking if next one is also CRLF
	it++;
	if (it->type == CRLF)
	{
		//must mean completed header section in full request therefore parse
		this->_parse();
		if (this->_method == "POST" && this->_contentLength > 0)
			return (this->_contentLength - this->_bytesRead);
		else
			return (-1);
	}
	it--;

	//if not full request, check if simple request or need to receive further
	try
	{
		this->_parseSimpleRequest(); //check if simple request	
	}
	catch(const std::exception& e)
	{
		std::cout << "not a simple request because: " << e.what() << std::endl;
		if (this->_tokenList.rbegin()->type == WORD && this->_tokenList.rbegin()->type == SPACE)
		{	
			data.insert(data.begin(), this->_tokenList.rbegin()->val.begin(), this->_tokenList.rbegin()->val.end());
			this->_tokenList.pop_back();
		}
		return (0);
	}
	//we don't care if stuff left after valid simple request
	return (-1);	
}


//Returns -1 if request complete, 0 if expecting more headers, # of bytes to read until end of header.
//Don't call again if -1 or >0
//Consumes data such that data=="leftover after CRLF" after each call.
int	Request::lexRawData(std::string &data)
{
	if (data == "")
		throw(std::runtime_error("empty data field"));

	this->_lexInput(data);
	data.clear();
	t_reqToken token = this->_tokenList.back();
	if (token.type == CRLF)
		return (_requestEval(data));
	if (token.type == WORD || token.type == SPACE)
	{
		//makes sure we don't have incomplete tokens in our list, we put them back in data
		data.insert(data.begin(), token.val.begin(), token.val.end());
		this->_tokenList.pop_back();
	}
	return (_requestEval(data));
}

std::string const	&Request::getMethod(void) const
{
	return (this->_method);
}

std::string const	&Request::getURI(void) const
{
	return (this->_URI);
}
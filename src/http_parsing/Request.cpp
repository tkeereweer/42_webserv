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

int		Request::_requestEval(std::string &data)
{
	std::list<t_reqToken>::const_reverse_iterator it = this->_tokenList.rbegin();

	for (it; it->type != CRLF && it != this->_tokenList.rbegin(); it++)

	//insert last token that could have been cut short
	if (it == this->_tokenList.rend() && (this->_tokenList.rbegin()->type == WORD && this->_tokenList.rbegin()->type == SPACE))
	{
		data.insert(data.begin(), this->_tokenList.rbegin()->val.begin(), this->_tokenList.rbegin()->val.end());
		this->_tokenList.pop_back();
	}

	if ((it + 1)->type == CRLF) //what if it + 1 is null ?
	{
		this->_reqComplete == true;
		_parse();
		return (_leftToRead());
	}

	try
	{
		_parseSimpleRequest();
		
	}
	catch(const std::exception& e)
	{
		if (this->_tokenList.rbegin()->type == WORD && this->_tokenList.rbegin()->type == SPACE)
		{	
			data.insert(data.begin(), this->_tokenList.rbegin()->val.begin(), this->_tokenList.rbegin()->val.end());
			this->_tokenList.pop_back();
		}
		return (0);
	}
	return (-1);	
	//check if stuff left after single CRLF	
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
		data.insert(data.begin(), token.val.begin(), token.val.end());
		this->_tokenList.pop_back();
	}
	return (_requestEval(data));
}











int main(int argc, char **argv)
{
	std::string test(argv[1]);

	try
	{
		
	}
	catch(const std::exception& e)
	{
		std::cerr << e.what() << '\n';
	}
	
	return (0);
}

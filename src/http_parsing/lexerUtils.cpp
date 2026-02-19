#include "../../include/Request.hpp"

//if last token is a word or space, don't read it as it will be put back in data.
long long	Request::_readLeftovers(std::list<t_reqToken>::iterator &it)
{
	std::string	bodyOverflow = "";
	std::list<t_reqToken>::iterator endPoint = this->_tokenList.end();

	//if endpoint is a potentially incomplete token, dont't write it in tempfile.
	//it will be put back in data by requestEval
	if (this->_tokenList.back().type == WORD || this->_tokenList.back().type == SPACE)
		endPoint--;

    //append to bodyOverflow all the body tokens
	for (std::list<t_reqToken>::iterator ite = it; ite != endPoint; ite++)
		bodyOverflow.append(ite->val.begin(), ite->val.end());

	long long	ret = this->_contentLength - bodyOverflow.size();
	if (ret < 0)
		throw (std::runtime_error("leftToRead < 0 !"));
	if (this->_bodyFilename == "")
		_createTempFile();
	std::ofstream	file(this->_bodyFilename.c_str());
	file.write(bodyOverflow.c_str(), bodyOverflow.size());
	this->_bytesRead = bodyOverflow.size();
	return (ret);
}

int	Request::_trySimpleRequest(std::string &data)
{
	try
	{
		_parseSimpleRequest();
		data.clear();
		return (-1);
	}
	catch(const std::exception& e)
	{
		std::cerr << e.what() << '\n';
		data.clear();
        //is this necessary ?
		if (this->_tokenList.back().type == WORD || this->_tokenList.back().type == SPACE)
		{
			data.insert(data.begin(), this->_tokenList.back().val.begin(), this->_tokenList.back().val.end());
			this->_tokenList.pop_back();
		}
		return (0);
	}	
}

int	Request::_tryFullParsing(std::string &data)
{
	_parse();
	if (this->_reqComplete)
		return (-1);
	else
	{
		data.clear();
		if (this->_tokenList.back().type == WORD || this->_tokenList.back().type == SPACE)
		{
			data.insert(data.begin(), this->_tokenList.back().val.begin(), this->_tokenList.back().val.end());
			this->_tokenList.pop_back();
		}
		return (this->_contentLength - this->_bytesRead);
	}	
}

int		Request::_requestEval(std::string &data)
{
	std::list<t_reqToken>::reverse_iterator	rit = this->_tokenList.rbegin();
	if (rit->type == CRLF)
	{
		rit++;
		if (rit->type == CRLF)
			return (_tryFullParsing(data));
		else
			return (_trySimpleRequest(data));
	}
	else
	{
		while (rit->type != CRLF)
			rit++;
		if (rit == this->_tokenList.rend()) //no CRLF
			return (0);
		rit++;
		if (rit->type == CRLF)
			return (_tryFullParsing(data));
		rit--;
		return (_trySimpleRequest(data));	
	}
}
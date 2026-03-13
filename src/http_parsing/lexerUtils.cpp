#include "Request.hpp"

//if last token is a word or space, don't read it as it will be put back in data.
void	Request::_readLeftovers(std::list<t_reqToken>::iterator &it)
{
	std::string	bodyOverflow = "";
	t_reqToken  tmp = this->_tokenList.back();

	//append to bodyOverflow all the body tokens
	for (std::list<t_reqToken>::iterator ite = it; ite != this->_tokenList.end(); ite++)
		bodyOverflow.append(ite->val.begin(), ite->val.end()); //this shit adds too much stuff back to readbuffer

	long long	ret = this->_contentLength - bodyOverflow.size();
	if (ret < 0)
		throw (Request::ErrorNum("content-length < body size", 400)); //should trigger 
	//create temp file
	if (this->_bodyFilename == "")
		_createTempFile();
	std::ofstream	file(this->_bodyFilename.c_str(), std::ios_base::binary);

	file.write(bodyOverflow.c_str(), bodyOverflow.size());
	this->_bytesRead = bodyOverflow.size();
	//if everything written in file, we're good !
	if (ret == 0)
		this->_reqComplete = true;
	return ;
}

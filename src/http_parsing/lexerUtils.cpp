#include "../../include/Request.hpp"

//if last token is a word or space, don't read it as it will be put back in data.
void	Request::_readLeftovers(std::list<t_reqToken>::iterator &it)
{
	std::string	bodyOverflow = "";
	t_reqToken  tmp = this->_tokenList.back();

	//append to bodyOverflow all the body tokens
	for (std::list<t_reqToken>::iterator ite = it; ite != this->_tokenList.end(); ite++)
		bodyOverflow.append(ite->val.begin(), ite->val.end());

	long long	ret = this->_contentLength - bodyOverflow.size();
	if (ret < 0)
		throw (std::runtime_error("leftToRead < 0 !"));
	//create temp file
	if (this->_bodyFilename == "")
		_createTempFile();
	std::ofstream	file(this->_bodyFilename.c_str());

	//if haven't read it all, don't write last potentially incomplete token
	//put back in data buffer up the stack.
	size_t sizeToWrite = bodyOverflow.size();
	if (ret != 0 && (tmp.type == WORD || tmp.type == SPACE))
		sizeToWrite -= tmp.val.size();

	file.write(bodyOverflow.c_str(), sizeToWrite);
	this->_bytesRead = bodyOverflow.size();
	//if everything written in file, we're good !
	if (ret == 0)
		this->_reqComplete = true;
	return ;
}


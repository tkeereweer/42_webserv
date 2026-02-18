#include "../../include/Request.hpp"

long long	Request::_readLeftovers(void)
{
	std::string	bodyOverflow = "";
	for (std::list<t_reqToken>::iterator it = this->_tokenList.begin();
			it != this->_tokenList.end();
			it++)
	{
		bodyOverflow.append(it->val.begin(), it->val.end());
	}
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
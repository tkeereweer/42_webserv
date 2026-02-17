#include "../../include/Request.hpp"

int	Request::_leftToRead(void)
{
	for (std::list<t_reqToken>::iterator it = this->_tokenList.begin();
			it != this->_tokenList.end();
			it++)
	{
		this->_entityBody.append(it->val.begin(), it->val.end());
	}
	long	ret = this->_contentLength - this->_entityBody.size();
	if (ret < 0)
		throw (std::runtime_error("leftToRead < 0 !"));
	return (ret);
}
#ifndef CLIENT_HPP
#define CLIENT_HPP

#include <string>
#include "Request.hpp"

class	Client
{
	private:
		int			_fd;
		std::string _readBuffer;
		Request     _request;
		std::string	_response; // same
		size_t		_bytesSent;
        bool        _requestDone;
        bool        _responseDone;

	public:
		Client(void);
		Client(int fd);
		Client(Client const &src);
		Client	&operator=(Client const &rhs);
		~Client(void);

		int					getFd(void) const;
		std::string&		getReadBuffer(void);
		const std::string&	getResponse(void) const;
		size_t				getBytesSent(void) const;
		Request             &getRequest(void);

		void				setReadBuffer(const std::string& readBuffer);
		void				setResponse(const std::string& response);
		void				appendReadBuffer(const std::string& appendix);
		void				addBytesSent(size_t bytesSent);

		void				clearReadBuffer(void);
		void				clearResponse(void);
		void				clearBytesSent(void);
};

#endif
#ifndef CLIENT_HPP
#define CLIENT_HPP

# include "libraryHeader.hpp"
# include "Request.hpp"
# include "Response.hpp"

class	Client
{
	private:
		int			_fd;
		std::string _readBuffer;
		Request     _request;
		Response	_response;
		size_t		_bytesSent;
		bool        _requestDone;
		int	        _cgiResponseState;
        std::time_t _firstCoTimestamp;

	public:
		Client(void);
		Client(int fd);
		Client(Client const &src);
		Client	&operator=(Client const &rhs);
		~Client(void);

		int					getFd(void) const;
		std::string&		getReadBuffer(void);
		Response&           getResponse(void);
		size_t				getBytesSent(void) const;
		Request             &getRequest(void);
		int					getCgiResponseState(void) const;
        std::time_t         getFirstCoTimestamp(void) const;

		void				setReadBuffer(const std::string& readBuffer);
		void				setResponse(const Response& response);
		void				appendReadBuffer(const std::string& appendix);
		void				addBytesSent(size_t bytesSent);
		void				setCgiResponseState(int state);
        void                setFirstCoTimestamp(std::time_t);

		void				clearReadBuffer(void);
		void				clearBytesSent(void);
};

#endif
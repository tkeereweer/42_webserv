#ifndef CLIENT_HPP
#define CLIENT_HPP

#include <string>

class	Client
{
	private:
		int			_fd;
		std::string	_request; //later: class
		std::string	_response; // same

	public:
		Client(void);
		Client(int fd);
		Client(Client const &src);
		Client	&operator=(Client const &rhs);
		~Client(void);

		int			getFd(void) const;
		std::string	getRequest(void) const;
		std::string	getResponse(void) const;

		void		setRequest(const std::string& request);
		void		setResponse(const std::string& response);
		void		appendRequest(const std::string& appendix);

		void		clearRequest(void);
		void		clearResponse(void);
};

#endif
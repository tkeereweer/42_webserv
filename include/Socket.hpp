#ifndef SOCKET_HPP
# define SOCKET_HPP

# include <string>

class	Socket
{
	private:
		std::string	_port;
		std::string	_ipAddr;

	public:
		Socket(void);
		Socket(std::string port, std::string ip);
		Socket(Socket const &src);
		Socket	&operator=(Socket const &rhs);
		~Socket(void);

		std::string	getPort(void) const;
};

#endif
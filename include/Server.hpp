#ifndef SERVER_HPP
# define SERVER_HPP

# include <string>
# include <vector>

# include "Location.hpp"

typedef struct s_socket
{
	std::string	port;
	std::string	ipAddr;
}	t_socket;


class	Server
{
	private:
		std::string				_name;
		std::vector<t_socket>	_sockets;
		int						_maxBodySizeClientReq;
		std::vector<Location>	_locations;
		std::string				_root;

	public:
		Server(void);
		Server(Server const &src);
		Server	&operator=(Server const &rhs);
		~Server(void);

		std::vector<t_socket>	&getSockets(void);

		void	addSocket(t_socket socket);
};

#endif
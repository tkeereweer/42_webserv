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
		unsigned long long		_maxBodySizeClientReq;
		std::vector<Location>	_locations;
		std::string				_root;

	public:
		Server(void);
		Server(Server const &src);
		Server	&operator=(Server const &rhs);
		~Server(void);

		std::string	getName(void) const;
		std::vector<t_socket>	&getSockets(void);
		unsigned long long	getMaxBody(void) const;
		std::vector<Location>	&getLocations(void);
		std::string	getServerRoot(void) const;


		void	setName(std::string name);
		void	setMaxBody(int maxBody);
		void	setServerRoot(std::string root);

		void	addSocket(t_socket socket);
		void	addLocation(Location location);
};

#endif
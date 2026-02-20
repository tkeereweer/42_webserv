#ifndef SERVER_HPP
# define SERVER_HPP

# include <string>
# include <vector>
# include <map>
# include <utility>

# include "Config.hpp"
# include "Location.hpp"

typedef struct s_socket
{
	std::string	port;
	std::string	ipAddr;
}	t_socket;


class	Server: public Config
{
	private:
		std::string					_name;
		std::vector<t_socket>		_sockets;
		std::vector<Location>		_locations;

	public:
		Server(void);
		Server(Server const &src);
		Server	&operator=(Server const &rhs);
		~Server(void);

		std::string					getName(void) const;
		std::vector<t_socket>		&getSockets(void);
		std::vector<Location>		&getLocations(void);

		void	setName(std::string name);
		void	addSocket(t_socket socket);
		void	addLocation(Location location);
};

#endif
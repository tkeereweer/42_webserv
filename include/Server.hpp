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
		long long				_maxBodySizeClientReq;
		std::vector<Location>	_locations;
		std::string				_root;

	public:
		Server(void);
		Server(Server const &src);
		Server	&operator=(Server const &rhs);
		~Server(void);

		std::string				getName(void) const;
		std::vector<t_socket>	&getSockets(void);
		long long		getMaxBody(void) const;
		std::vector<Location>	&getLocations(void);
		std::string				getServerRoot(void) const;


		void	setName(std::string name);
		void	setMaxBody(long long maxBody);
		void	setServerRoot(std::string root);
		void	addSocket(t_socket socket);
		void	addLocation(Location location);
};

#endif
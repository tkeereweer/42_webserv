#ifndef SERVER_HPP
# define SERVER_HPP

# include <string>

# include "Socket.hpp"
# include "Location.hpp"

class	Server
{
	private:
		std::string	_name;
		int			_numSockets;
		Socket		*_sockets;
		int			_maxBodySizeClientReq;
		int			_numLocations;
		Location	*_locations;
		std::string	_root;

	public:
		Server(void);
		Server(Server const &src);
		Server	&operator=(Server const &rhs);
		~Server(void);

		int		getNumSockets(void) const;
		Socket	getSocket(int i) const;

		void	setSocket(Socket socket, int pos);
};

#endif
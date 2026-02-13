#ifndef WEBSERV_HPP
# define WEBSERV_HPP

# include "Server.hpp"
# include <map>

class	Webserv
{
	private:
		int						_numServers;
		Server					*_servers;
		std::map<int, Server>	_serverMap;

	public:
		Webserv(void);
		Webserv(unsigned int numServers);
		Webserv(Webserv const &src);
		Webserv	&operator=(Webserv const &rhs);
		~Webserv(void);

		void	setServer(Server server, int pos);

		void	openSockets(void);
};

#endif
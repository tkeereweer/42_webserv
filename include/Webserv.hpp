#ifndef WEBSERV_HPP
# define WEBSERV_HPP

# include "Server.hpp"
# include "Client.hpp"
# include <map>

class	Webserv
{
	private:
		int						_numServers;
		Server					*_servers;
		std::map<int, Server>	_serverMap;
		std::map<int, Client*>	_clientMap;
		int						_epollFd;

		int		setupEpoll(void) const;

		bool	isListenSocket(int fd) const;
		void	newClient(int listenFd);
    	void	handleRequest(int clientFd);
		void	handleResponse(int clientFd);
    	void	closeClient(int clientFd);

	public:
		Webserv(void);
		Webserv(unsigned int numServers);
		Webserv(Webserv const &src);
		Webserv	&operator=(Webserv const &rhs);
		~Webserv(void);

		void	setServer(Server server, int pos);

		void	openSockets(void);
		void	launchServer(void);
};

#endif
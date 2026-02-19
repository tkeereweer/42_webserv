#ifndef WEBSERV_HPP
# define WEBSERV_HPP

# include "Server.hpp"
# include "Client.hpp"
# include <map>
# include <vector>

typedef struct	s_connection
{
	Client	client;
	Server*	server;
}	t_connection;

class	Webserv
{
	private:
		std::vector<Server>			_servers;
		std::map<int, Server*>		_serverMap;
		std::map<int, t_connection>	_clientMap;
		int							_epollFd;

		int		setupEpoll(void) const;

		bool	isListenSocket(int fd) const;
		void	newClient(int listenFd);
    	void	handleRequest(int clientFd);
		void	handleResponse(int clientFd);
    	void	closeClient(int clientFd);

		void	testPrint(int clientFd);

	public:
		Webserv(void);
		Webserv(Webserv const &src);
		Webserv	&operator=(Webserv const &rhs);
		~Webserv(void);

		void	addServer(Server server);

		void	openSockets(void);
		void	launchServer(void);
};

#endif
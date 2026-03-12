#ifndef WEBSERV_HPP
# define WEBSERV_HPP

# include "Config.hpp"
# include "Server.hpp"
# include "Client.hpp"
# include "Request.hpp"
# include "libraryHeader.hpp"

# define QUERY_TIMEOUT 30 //NGINX does 30-60s
# define CGI_TIMEOUT 30
# define FIRST_CONNEXION_TIMEOUT 55 //time from accepted client

typedef enum	e_conf_type
{
	CONF_WORD,
	OPEN_CURLY,
	CLOSE_CURLY,
	SEMICOLON
}	t_conf_type;

typedef	struct	s_conf_token
{
	t_conf_type	type;
	std::string	value;
}	t_conf_token;

typedef struct	s_connection
{
	Client	client;
	Server*	server;
}	t_connection;

class	Webserv
{
	private:
		std::vector<Server>			_servers;
		std::map<int, Server*>		_serverMap;// matches socketfd w/ corresponding server
		std::map<int, t_connection>	_clientMap; //int: fd of connection
		int							_epollFd;
		std::vector<std::string>	_parentEnv;

		// configuration file parsing
		void					parseListen(std::list<t_conf_token>::iterator &token, std::list<t_conf_token>::iterator &end, Server &server);
		void					parseServerName(std::list<t_conf_token>::iterator &token, std::list<t_conf_token>::iterator &end, Server &server);
		void					parseMaxBodySize(std::list<t_conf_token>::iterator &token, std::list<t_conf_token>::iterator &end, Config &config);
		void					parseRoot(std::list<t_conf_token>::iterator &token, std::list<t_conf_token>::iterator &end, Config &config);
		void					parseLimitExcept(std::list<t_conf_token>::iterator &token, std::list<t_conf_token>::iterator &end, Config &config);
		void					parseErrorPage(std::list<t_conf_token>::iterator &token, std::list<t_conf_token>::iterator &end, Config &config);
		void					parseAutoIndex(std::list<t_conf_token>::iterator &token, std::list<t_conf_token>::iterator &end, Config &config);
		void					parseIndex(std::list<t_conf_token>::iterator &token, std::list<t_conf_token>::iterator &end, Config &config);
		void					parseUpload(std::list<t_conf_token>::iterator &token, std::list<t_conf_token>::iterator &end, Location &location);
		void					parseRedir(std::list<t_conf_token>::iterator &token, std::list<t_conf_token>::iterator &end, Config &config);
		void					parseMaxCGIOutput(std::list<t_conf_token>::iterator &token, std::list<t_conf_token>::iterator &end, Config &config);
		void					parseLocation(std::list<t_conf_token>::iterator &token, std::list<t_conf_token>::iterator &end, Location &location);
		void					parseServerBlock(std::list<t_conf_token>::iterator &token, std::list<t_conf_token>::iterator &end, Server &server);
		std::string				openFile(char const *filepath);
		std::list<t_conf_token>	lexConfigFile(std::string content);
		void					printConfTokens(std::list<t_conf_token> lst);
		void					parseConfTokens(std::list<t_conf_token> &tokens);

		// socket/connection management
		int		setupEpoll(void) const;
		void	activityNotif(struct epoll_event	readyEvents);
		bool	isListenSocket(int fd) const;
		void	newClient(int listenFd);
		void	handleRequest(int clientFd);
		void	handleResponse(int clientFd);
		void    handleTimeouts(void);
        void    handleCgiTimeout(std::time_t &now);
		void	closeClient(int clientFd);
		
		// HandleCgi
		long        isCgiFd(int fd);
		void		_handleCgiInput(CGI &cgi, Server &server);
		void		_handleCgiOutput(CGI &cgi, Server &server);
        void	    _handleErrorPipe(CGI &cgi);
		void		_cgiError(CGI &cgi);
		int			_setupCGIResponseHeaders(CGI &cgi, long long maxOutSize);
		void        _destroyCGI(CGI &cgi, Server &server);
		void		_buildOtherCode(CGI &cgi);
		void		_cleanExit(void);


		void	testPrint(int clientFd, Client &client);
		Webserv(void);

	public:
		Webserv(char **envp);
		Webserv(Webserv const &src);
		Webserv	&operator=(Webserv const &rhs);
		~Webserv(void);

		std::vector<Server>		&getServers(void);
		std::map<int, Server*>	&getServerMap(void);
		int						getEpollFd(void) const;

		void	addServer(Server server);

		void	getConfig(char const *filepath);
		
		void	openSockets(void);
		void	launchServer(void);
};

std::ostream	&operator<<(std::ostream &o, Webserv &input);

#endif
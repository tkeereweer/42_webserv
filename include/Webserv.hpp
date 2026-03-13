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
		void					_parseListen(std::list<t_conf_token>::iterator &token, std::list<t_conf_token>::iterator &end, Server &server);
		void					_parseServerName(std::list<t_conf_token>::iterator &token, std::list<t_conf_token>::iterator &end, Server &server);
		void					_parseMaxBodySize(std::list<t_conf_token>::iterator &token, std::list<t_conf_token>::iterator &end, Config &config);
		void					_parseRoot(std::list<t_conf_token>::iterator &token, std::list<t_conf_token>::iterator &end, Config &config);
		void					_parseLimitExcept(std::list<t_conf_token>::iterator &token, std::list<t_conf_token>::iterator &end, Config &config);
		void					_parseErrorPage(std::list<t_conf_token>::iterator &token, std::list<t_conf_token>::iterator &end, Config &config);
		void					_parseAutoIndex(std::list<t_conf_token>::iterator &token, std::list<t_conf_token>::iterator &end, Config &config);
		void					_parseIndex(std::list<t_conf_token>::iterator &token, std::list<t_conf_token>::iterator &end, Config &config);
		void					_parseUpload(std::list<t_conf_token>::iterator &token, std::list<t_conf_token>::iterator &end, Location &location);
		void					_parseRedir(std::list<t_conf_token>::iterator &token, std::list<t_conf_token>::iterator &end, Config &config);
		void					_parseMaxCGIOutput(std::list<t_conf_token>::iterator &token, std::list<t_conf_token>::iterator &end, Config &config);
		void					_parseLocation(std::list<t_conf_token>::iterator &token, std::list<t_conf_token>::iterator &end, Location &location);
		void					_parseServerBlock(std::list<t_conf_token>::iterator &token, std::list<t_conf_token>::iterator &end, Server &server);
		std::string				_openFile(char const *filepath);
		std::list<t_conf_token>	_lexConfigFile(std::string content);
		void					_printConfTokens(std::list<t_conf_token> lst);
		void					_parseConfTokens(std::list<t_conf_token> &tokens);
		void					_testErrorPages(Server &server);

		// socket/connection management
		int		_setupEpoll(void) const;
		void	_activityNotif(struct epoll_event	readyEvents);
		bool	_isListenSocket(int fd) const;
		void	_newClient(int listenFd);
		void	_handleRequest(int clientFd);
		int		_lexInput(Client &client, int clientFD);
		void	_handleResponse(int clientFd);
		void    _handleTimeouts(void);
		void    _handleCgiTimeout(std::time_t &now);
		void	_closeClient(int clientFd);
		void	_modifyEpoll(int EVENT, int MODIFIER, int whichFd);
		
		// HandleCgi
		long        _isCgiFd(int fd);
		void		_handleCgiInput(CGI &cgi, Server &server);
		void		_handleCgiOutput(CGI &cgi, Server &server);
		void	    _handleErrorPipe(CGI &cgi);
		void		_returnValidCgi(CGI &cgi);
		void		_setupInvalidCgi(CGI &cgi);
		void		_returnError500(CGI &cgi, Client &client);
		void		_cgiError(CGI &cgi);
		int			_setupCGIResponseHeaders(CGI &cgi, long long maxOutSize);
		void        _destroyCGI(CGI &cgi, Server &server);
		void		_buildOtherCode(CGI &cgi);
		void		_cleanExit(void);

	public:
		Webserv(void);
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
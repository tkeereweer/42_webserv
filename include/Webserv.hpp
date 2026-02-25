#ifndef WEBSERV_HPP
# define WEBSERV_HPP

# include "Config.hpp"
# include "Server.hpp"
# include "Client.hpp"
# include "Request.hpp"
# include <map>
# include <vector>
# include <list>
# include <ostream>
# include <ctime>

# define QUERY_TIMEOUT 300000 //30s == 30000ms between 2 reads and write is the standard on firefox. If lower, test value for debugging quicker

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

typedef	struct s_cgi
{
	int			clientFd;
	int			pid;
	int			writeFd;
	int 		readFd;
	int			inFileFd;
	int			outFileFd;
	std::string	outFile;
	ssize_t		bytesSent;
}	t_cgi;

class	Webserv
{
	private:
		std::vector<Server>			_servers;
		std::map<int, Server*>		_serverMap;
		std::map<int, t_cgi>		_cgiMap;
		std::map<int, t_connection>	_clientMap; //int: fd of connection
		int							_epollFd;

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
    	void	closeClient(int clientFd);
		
		// HandleCgi
		bool		isCgiFd(int fd);
		void		launchCgi(int clientFd);
		void		createPipes(int inPipe[], int outPipe[]);
		t_cgi		populateCgiStruct(int clientFd, pid_t pid, int *inPipe, int *outPipe, std::map<int, t_cgi>& _cgiMap);
		void		childProcessCgi(int *inPipe, int *outPipe, int clientFd);
		char**		setupEnv(int clientFd);
		void		freeEnv(char **env);
		void		addCgiToEpoll(t_cgi cgi);
		void		handleCgiInput(int writeFd);
		void		handleCgiOutput(int readFd);
		void		closeCgi(int readFd);
		void    	_createTempFile(t_cgi& cgi);
		void		_createNextAvailableFile(struct dirent *name, DIR *tmp, t_cgi& cgi);
		std::string	getScriptPath(int clientFd);
		std::string	getProgPath(std::string scriptPath);


		void	testPrint(int clientFd, Client &client);

	public:
		Webserv(void);
		Webserv(Webserv const &src);
		Webserv	&operator=(Webserv const &rhs);
		~Webserv(void);

		std::vector<Server>		&getServers(void);
		std::map<int, Server*>	&getServerMap(void);

		void	addServer(Server server);

		void	getConfig(char const *filepath);
		
		void	openSockets(void);
		void	launchServer(void);
};

std::ostream	&operator<<(std::ostream &o, Webserv &input);

#endif
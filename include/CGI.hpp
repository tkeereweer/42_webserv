#ifndef CGI_HPP
# define CGI_HPP

# include "libraryHeader.hpp"
# include "Request.hpp"
# include "Location.hpp"

class Client;

typedef enum	e_cgi_token
{
	CGI_WORD,
	CGI_QUOTED,
	CGI_SPACE,
	CGI_LB,
	CGI_COLON,
	CGI_SLASH,
	CGI_SEMI_COLON
}	t_cgi_token;

typedef struct s_cgiToken
{
	t_cgi_token type;
	std::string	val;
}	t_cgiToken;

class CGI
{
	private:
		Location					*_loc;
		int							_clientFd;
		int							_pid;
		int							_writeFd;
		int 						_readFd;
		int							_inFileFd; //tempfile where the body is stored.
		std::string					_outBuff;
		ssize_t						_bytesSent;
		std::string					_scriptPath;
		//environment variables
		std::vector<std::string>	_cgiEnv;
		std::string					_queryString; //parsed by handle post in Server::
		//CGI output
		std::list<t_cgiToken>	_CGItokenList;
		long long				_contentLength;
		std::string				_contentType;
		int						_status;
		//flag to indicate we got to the end of output headers
		bool	_outComplete;
		bool	_outHeadersValid;
		//body bytes already read such that after body consumed, contentLength - bytesRead == 0
		long long		_bytesRead;
		struct timeval	_outTimestamp;

		void		_createChildProcess(int *inPipe, int *outPipe, char **childEnv);
        void	    _setupEnvPOST(std::vector<std::string> env, char ***childEnv, Client &client);
        void	    _setupEnvGET(std::string queryString, std::vector<std::string> env, char ***childEnv, Client &client);

		void		_lexInput(std::string const &str);
		void		_parseCGIOutput(std::list<t_cgiToken>::iterator &it);
		bool		_parseContentLength(std::list<t_cgiToken>::iterator &it);
		bool		_parseContentType(std::list<t_cgiToken>::iterator &it);
		bool		_parseStatus(std::list<t_cgiToken>::iterator &it);
		std::string	_parseMediaType(std::list<t_cgiToken>::iterator &it);
		void		_readLeftovers(std::list<t_cgiToken>::iterator &it);

		CGI(void);
	public:
		//post method constructor
		CGI(std::vector<std::string> env, Client &client, Location *loc, std::string scriptPath);
		//get method constructor
		CGI(std::string queryString, std::vector<std::string> env, Client &client, Location *loc, std::string scriptPath);
		CGI(CGI const &src);
		~CGI(void);
		CGI	&operator=(CGI const &rhs);

        void	closeCgi(int epollFD);

		//getters
		int				getClientFD(void) const;
		int 			getPID(void) const;
		int				getWriteFD(void) const;
		int				getReadFD(void) const;
		std::string		&getOutBuff(void);
		ssize_t			getBytesSent(void) const;
		long long		getContentLength(void) const;
		std::string		getContentType(void) const;
		Location		&getLocation(void);
		struct timeval	getOutTimestamp(void) const;
		int				getStatus(void) const;

		//setters
		void	setCGIContentLength(long long length);
		void	setCGIContentType(std::string type);
		void	addBytesSent(int bytes);
		void	setOutTimestamp(struct timeval tv);

		//parse CGI output
		int	lexCGIOutput(std::string &data);
};

#endif
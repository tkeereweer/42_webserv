#ifndef CGI_HPP
# define CGI_HPP

# include "Client.hpp"
# include <string>
# include <iostream>
# include <list>

typedef enum	e_cgi_token
{
	CGI_WORD,
	CGI_QUOTED,
	CGI_SPACE,
	CGI_LB,
	CGI_COLON
}	t_cgi_token;

class CGI //cgi's map key is clientFD
{
	private:
		int			_clientFd;
		int			_pid;
		int			_writeFd;
		int 		_readFd;
		int			_inFileFd; //tempfile where the body is stored.
		std::string	_outBuff;
		ssize_t		_bytesSent;
		std::string	_scriptPath;
		//environment variables
		char		**_cgiEnv;
		std::string	_queryString; //parsed by handle post in Server::
		//CGI output
		std::list<t_reqToken>	_CGItokenList;
		long long				_contentLength;
		std::string				_contentType;
		//flag to indicate we got to the end of request headers
		bool    _outComplete;
		bool    _outLineValid;      
		bool    _outHeadersValid;         
		//body bytes already read such that after body consumed, contentLength - bytesRead == 0
		long long	    _bytesRead;
		struct timeval	_outTimestamp;

		void		_createChildProcess(int *inPipe, int *outPipe, const char **childEnv);
        void	    _setupEnvPOST(char **env, const char **childEnv, Client &client);
        void	    _setupEnvGET(std::string queryString, char **env, const char **childEnv, Client &client);

		void	_lexInput(std::string const &str);

		CGI(void);
	public:
		//post method constructor
		CGI(char **env, Client &client, std::string scriptPath);
		//get method constructor
		CGI(std::string queryString, char **env, Client &client, std::string scriptPath);
		CGI(CGI const &src);
		~CGI(void);
		CGI	&operator=(CGI const &rhs);

        void	closeCgi(int epollFD);

		//getters
		int			getClientFD(void) const;
		int 		getPID(void) const;
		int			getWriteFD(void) const;
		int			getReadFD(void) const;
		std::string	&getOutBuff(void);
		ssize_t		getBytesSent(void) const;

		//setters
		void	setCGIContentLength(long long length);
		void	setCGIContentType(std::string type);

		//parse CGI output
		int	lexCGIOutput(std::string &data);
};

#endif
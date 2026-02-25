#ifndef CGI_HPP
# define CGI_HPP

#include "Client.hpp"
#include <string>
#include <iostream>

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

		void		_createChildProcess(int *inPipe, int *outPipe);
		std::string	_getProgPath(std::string scriptPath);

		CGI(void);
	public:
		//post method constructor
		CGI(char **env, Client &client, std::string scriptPath);
		//get method constructor
		CGI::CGI(std::string queryString, char **env, Client &client, std::string scriptPath);
		CGI(CGI const &src);
		~CGI(void);
		CGI	&operator=(CGI const &rhs);

		//getters
		int			getClientFD(void) const;
		int 		getPID(void) const;
		int			getWriteFD(void) const;
		int			getReadFD(void) const;
		std::string	&getOutBuff(void);
		ssize_t		getBytesSent(void) const;
};

#endif
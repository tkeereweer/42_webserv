#include "CGI.hpp"
#include "../include/Webserv.hpp"
#include <sys/wait.h>
#include <fcntl.h>
#include <sys/epoll.h>
#include <cstring>
#include <dirent.h>

CGI::CGI(void){}

//constructor for get method CGI with query_string argument
CGI::CGI(std::string queryString, char **env, Client &client, std::string scriptPath):
	_cgiEnv(env),
	_clientFd(client.getFd()),
	_queryString(queryString),
	_scriptPath(scriptPath),
	_bytesSent(0),
	_writeFd(-1),
	_inFileFd(-1),
{
	//don't build input pipe but append queryString as query_string=<value here> to end of servEnv.

	//add what's below when switching back to relative paths
	// std::string dot(".");
	// this->_scriptPath.insert(this->_scriptPath.begin(), dot.begin(), dot.end());
}


//constructor for post method CGI
CGI::CGI(char **env, Client &client, std::string scriptPath):
	_cgiEnv(env),
	_clientFd(client.getFd()),
	_scriptPath(scriptPath),
	_bytesSent(0)
{
	//add what's below when switching back to relative paths
	// std::string dot(".");
	// this->_scriptPath.insert(this->_scriptPath.begin(), dot.begin(), dot.end());
	int	inPipe[2];
	int outPipe[2];
	if (pipe(inPipe) == -1)
		throw (std::runtime_error(std::strerror(errno)));
	if (pipe(outPipe) == -1)
	{
		close(inPipe[0]);
		close(inPipe[1]);
		throw (std::runtime_error(std::strerror(errno)));
	}

	//setup environment variables before here
	this->_pid = fork();
	if (this->_pid == -1)
		throw (std::runtime_error(std::strerror(errno)));
	if (this->_pid == 0)
		_createChildProcess(inPipe, outPipe);
	close(inPipe[0]);
	close(outPipe[1]);

	this->_readFd = outPipe[0];
	this->_writeFd = inPipe[1]; //only for post method
	this->_inFileFd = open(client.getRequest().getBodyFilename().c_str(), O_RDONLY);
	if (this->_inFileFd == -1)
			throw (std::runtime_error(std::strerror(errno)));
}


void	Webserv::addCgiToEpoll(t_cgi cgi)
{
	epoll_event	ev;
	ev.events = EPOLLIN;
	ev.data.fd = cgi.readFd;
	fcntl(cgi.readFd, F_SETFL, O_NONBLOCK);
	epoll_ctl(_epollFd, EPOLL_CTL_ADD, cgi.readFd, &ev);

	if (cgi.writeFd != -1)
	{
		ev.events = EPOLLOUT;
		ev.data.fd = cgi.writeFd;
		fcntl(cgi.writeFd, F_SETFL, O_NONBLOCK);
		epoll_ctl(_epollFd, EPOLL_CTL_ADD, cgi.writeFd, &ev);
	}
}



CGI::CGI(CGI const &src)
{
	*this = src;
}

CGI::~CGI(void)
{
	close(this->_inFileFd);
}

CGI	&CGI::operator=(CGI const &rhs)
{
	if (this != &rhs)
	{
		this->_clientFd = rhs._clientFd;
		this->_pid = rhs._clientFd;
		this->_writeFd = rhs._writeFd;
		this->_readFd = rhs._readFd;
		this->_inFileFd = rhs._inFileFd;
		this->_outBuff = rhs._outBuff;
		this->_bytesSent = rhs._bytesSent;
		this->_cgiEnv = rhs._cgiEnv;
		this->_queryString = rhs._queryString;
	}
	return (*this);
}

//getters
int			CGI::getClientFD(void) const
{
	return (this->_clientFd);
}

int 		CGI::getPID(void) const
{
	return (this->_pid);
}

int			CGI::getWriteFD(void) const
{
	return (this->_writeFd);
}

int			CGI::getReadFD(void) const
{
	return (this->_readFd);
}

std::string	&CGI::getOutBuff(void)
{
	return (this->_outBuff);
}

ssize_t		CGI::getBytesSent(void) const
{
	return (this->_bytesSent);
}


/*******************************************************************************
*						INIT
*******************************************************************************/

void	Webserv::launchCgi(int clientFd)
{
	int		inPipe[2];
	int		outPipe[2];

	createPipes(inPipe, outPipe);

	pid_t pid = fork();

	if (pid == -1)
		throw (std::runtime_error(std::strerror(errno)));

	if (pid == 0)
		childProcessCgi(inPipe, outPipe, clientFd);

	close(inPipe[0]);
	close(outPipe[1]);

	CGI	cgi = populateCgiStruct(clientFd, pid, inPipe, outPipe, _cgiMap);

	addCgiToEpoll(cgi);
}


void	CGI::_createChildProcess(int *inPipe, int *outPipe)
{
		dup2(inPipe[0], STDIN_FILENO);
		dup2(outPipe[1], STDOUT_FILENO);
		close(inPipe[1]);
		close(outPipe[0]);

		std::string	progPath = _getProgPath(this->_scriptPath);

		char		*path = const_cast<char*>(progPath.c_str());
		char		*argv[] = {	path,
								const_cast<char*>(this->_scriptPath.c_str()),
								NULL};
		// char		**env = setupEnv(clientFd);

		if (this->_cgiEnv == NULL || execve(path, argv, env) == -1)
		{
			freeEnv(env);
			close(inPipe[0]);
			close(outPipe[1]);
			exit(1);
		}
}

/*******************************************************************************
*						CHILD HELPERS
*******************************************************************************/

std::string	Webserv::getScriptPath(int clientFd)
{
	return ("." + _clientMap[clientFd].client.getRequest().getURI());
}


std::string	CGI::_getProgPath(std::string scriptPath)
{
	std::string ext = scriptPath.substr(scriptPath.find_last_of('.'));
	if (ext == ".py")
		return "/usr/bin/python3";
	if (ext == ".php")
		return "/usr/bin/php";
	throw(std::runtime_error(".py or .php only"));
}


char**	Webserv::setupEnv(int clientFd)
{
	Request&				request = _clientMap[clientFd].client.getRequest();

	//ADD MORE HEADERS OR INFO IF NEEDED IN THE SCRIPT
	std::vector<std::string> builder;
	builder.push_back("CONTENT_LENGTH=" + request.getContentLength());
	// builder.push_back("QUERY_STRING=" + request.getQuery());
	// builder.push_back("COOKIE=" + );

	int		size = builder.size();
	char	**dest = new char *[size + 1];

	for (int i = 0; i < size; i++)
		dest[i] = strdup(builder[i].c_str());
	dest[size] = 0;

	return (dest);
}


void	Webserv::freeEnv(char **env)
{
	if (!env || !(*env))
		return ;

	for (int i = 0; env[i]; i++)
		free(env[i]);

	free(env);
}


/*******************************************************************************
*						READ / WRITE / CLOSE
*******************************************************************************/
//is buffeer placeholder for where we want to write actually ?
void	Webserv::handleCgiInput(int writeFd)
{
	CGI	&cgi = _cgiMap[writeFd];
	long long contentLength = _clientMap[cgi.getClientFD()].client.getRequest().getContentLength();
	char	buffer[1024];

	//logic to change as we're either reading directly
	ssize_t bytesRead = read(cgi.inFileFd, buffer, sizeof(buffer) - 1);

	if (bytesRead > 0)
	{
		buffer[bytesRead] = 0;
		ssize_t bytesSentNow = write(writeFd, buffer, bytesRead);
		cgi.bytesSent += bytesSentNow;
		if (cgi.bytesSent >= contentLength)
		{
			close(cgi.inFileFd);
			epoll_ctl(_epollFd, EPOLL_CTL_DEL, writeFd, NULL);
			close(writeFd);
			_cgiMap.erase(writeFd);
			cgi.writeFd = -1;
		}
	}
	else
		closeCgi(_cgiMap[writeFd].readFd);
}

//is buffer placeholder for where we want to read ?
void	Webserv::handleCgiOutput(int readFd)
{
	t_cgi &cgi = _cgiMap[readFd];
	char buffer[4096];

	ssize_t bytesRead = read(readFd, buffer, sizeof(buffer));

	if (bytesRead > 0)
		write(cgi.outFileFd, buffer, bytesRead);
	else
		closeCgi(readFd);
}


void	Webserv::closeCgi(int readFd)
{
	t_cgi &cgi = _cgiMap[readFd];

	int	status;
	waitpid(cgi.pid, &status, WNOHANG); //non blocking 
	if (WIFEXITED(status) && WEXITSTATUS(status) != 0)
	{
		// script error,  return appropriate code
	}

	if (cgi.writeFd != -1)
	{
		close(cgi.inFileFd);
		epoll_ctl(_epollFd, EPOLL_CTL_DEL, cgi.writeFd, NULL);
		close(cgi.writeFd);
		_cgiMap.erase(cgi.writeFd);
		cgi.writeFd = -1;
	}

	//TEST START
	/* this test was to write the outfile in Response and send it to the client, using curl
		can be used with mini script <test.py> that simply prints headers and body passed to it

	*/

	// std::ifstream f(cgi.outFile.c_str());
	// std::ostringstream ss;
	// ss << f.rdbuf();
	// std::string body = ss.str();

	// std::ostringstream response;
	// response << "HTTP/1.1 200 OK\r\n";
	// response << "Content-Length: " << body.size() << "\r\n";
	// response << "Content-Type: text/plain\r\n";
	// response << "\r\n";
	// response << body;
	// _clientMap[cgi.clientFd].client.setResponse(response.str());

	// struct epoll_event event;
	// event.events = EPOLLOUT;
	// event.data.fd = cgi.clientFd;
	// int ret = epoll_ctl(_epollFd, EPOLL_CTL_ADD, cgi.clientFd, &event);

	//TEST END

	epoll_ctl(_epollFd, EPOLL_CTL_DEL, readFd, NULL);
	close(readFd);
	close(cgi.outFileFd);
	_cgiMap.erase(readFd);


	
}


/*******************************************************************************
*						TEMP FILE
*******************************************************************************/

// void    Webserv::_createTempFile(t_cgi& cgi)
// {
// 	if (cgi.outFile != "")
// 		return ;
	
// 	DIR *tmp = opendir("/tmp");
// 	if (tmp == NULL)
// 		throw(std::runtime_error(strerror(errno)));

// 	struct dirent *name = readdir(tmp);
// 	if (!name && errno != 0)
// 		throw(std::runtime_error(strerror(errno)));
// 	while (name && strncmp(name->d_name, "wbsrv_cgi_", strlen("wbsrv_cgi_")))
// 		name = readdir(tmp);
// 	if (errno != 0)
// 		throw(std::runtime_error(strerror(errno)));
// 	//no temp file, create from 0
// 	if (!name)
// 	{
// 		std::ofstream file("/tmp/wbsrv_cgi_0");
// 		if (!file)
// 			throw(std::runtime_error("can't open file"));
//         cgi.outFile = "/tmp/wbsrv_cgi_0";
// 		return ;
// 	}

// 	//handle next available file logic
// 	return (_createNextAvailableFile(name, tmp, cgi));
// }


// void	Webserv::_createNextAvailableFile(struct dirent *name, DIR *tmp, t_cgi& cgi)
// {
// 	std::string lastFile(name->d_name);
// 	int num = atoi(std::string(&lastFile[lastFile.find_last_of("_") + 1]).c_str()); //first num found
// 	int next_num = 0;
// 	std::ostringstream numWrite;
// 	std::string	nameToFind = "wbsrv_cgi_";

// 	//exit loop if no file found or intmax number of temp files
// 	while (name && next_num < std::numeric_limits<int>::max())
// 	{
// 		nameToFind = "wbsrv_cgi_";
// 		numWrite << next_num;
// 		nameToFind += numWrite.str();
// 		//finds file with nex number
// 		while (name && strncmp(name->d_name, nameToFind.c_str(), strlen(nameToFind.c_str())))
// 			name = readdir(tmp);
// 		next_num++;
// 		numWrite.str("");
// 	}
// 	if (errno != 0)
// 		throw(std::runtime_error(strerror(errno)));
// 	if (num == std::numeric_limits<int>::max())
// 		throw(std::runtime_error("out of room for temp files"));
	
// 	//create available file found
// 	std::string	filename("/tmp/");
// 	filename += nameToFind;
// 	std::ofstream file(filename.c_str());
// 	if (!file)
// 			throw(std::runtime_error("can't open file"));
//     cgi.outFile = filename;
// 	return ;	
// }

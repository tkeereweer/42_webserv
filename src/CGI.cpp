#include "../include/CGI.hpp"
#include "../include/Webserv.hpp"


CGI::CGI(void) {}

void	freeEnv(char **env);

//constructor for get method CGI with query_string argument
CGI::CGI(std::string queryString, std::vector<std::string> env, Client &client, Location *loc, std::string scriptPath):
	_loc(loc),
	_clientFd(client.getFd()),
	_writeFd(-1),
	_inFileFd(-1),
    _bytesSent(0),
    _scriptPath(scriptPath),
    _cgiEnv(env),
    _queryString(queryString),
	_contentLength(-1),
	_status(-1),
	_outComplete(false),
	_outHeadersValid(false)
{
	//don't build input pipe but append queryString as query_string=<value here> to end of servEnv.

	//add what's below when switching back to relative paths
	// std::string dot(".");
	// this->_scriptPath.insert(this->_scriptPath.begin(), dot.begin(), dot.end());
	int	inPipe[2];
	int outPipe[2];
	if (pipe(outPipe) == -1)
		throw (std::runtime_error(std::strerror(errno)));

	//setup environment variables
	char	**childEnv;
	_setupEnvGET(queryString, env, &childEnv, client);

	//create child
	this->_pid = fork();
	if (this->_pid == -1)
		throw (std::runtime_error(std::strerror(errno)));
	if (this->_pid == 0)
		_createChildProcess(inPipe, outPipe, childEnv);
	close(inPipe[0]);
	close(outPipe[1]);
	freeEnv(childEnv);

	this->_readFd = outPipe[0];
	// this->_inFileFd = open(client.getRequest().getBodyFilename().c_str(), O_RDONLY); //get has no body
	// if (this->_inFileFd == -1)
	// 	throw (std::runtime_error(std::strerror(errno)));

	this->_outTimestamp.tv_usec = 0;
	this->_outTimestamp.tv_sec = 0;
}

//constructor for post method CGI
CGI::CGI(std::vector<std::string> env, Client &client, Location *loc, std::string scriptPath):
	_loc(loc),
	_clientFd(client.getFd()),
	_bytesSent(0),
    _scriptPath(scriptPath),
    _cgiEnv(env),
	_contentLength(-1),
	_status(-1),
	_outComplete(false),
	_outHeadersValid(false)
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

	//setup environment variables
	char	**childEnv;
	_setupEnvPOST(env, &childEnv, client);

	//create child
	this->_pid = fork();
	if (this->_pid == -1)
		throw (std::runtime_error(std::strerror(errno)));
	if (this->_pid == 0)
		_createChildProcess(inPipe, outPipe, childEnv);
	close(inPipe[0]);
	close(outPipe[1]);
	freeEnv(childEnv);

	this->_readFd = outPipe[0];
	this->_writeFd = inPipe[1]; //only for post method
	this->_inFileFd = open(client.getRequest().getBodyFilename().c_str(), O_RDONLY);
	if (this->_inFileFd == -1)
		throw (std::runtime_error(std::strerror(errno)));

	this->_outTimestamp.tv_usec = 0;
	this->_outTimestamp.tv_sec = 0;
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
		this->_loc = rhs._loc;
		this->_clientFd = rhs._clientFd;
		this->_pid = rhs._clientFd;
		this->_writeFd = rhs._writeFd;
		this->_readFd = rhs._readFd;
		this->_inFileFd = rhs._inFileFd;
		this->_outBuff = rhs._outBuff;
		this->_bytesSent = rhs._bytesSent;
		this->_cgiEnv = rhs._cgiEnv;
		this->_queryString = rhs._queryString;
		this->_contentLength = rhs._contentLength;
		this->_contentType = rhs._contentType;
		this->_outComplete = rhs._outComplete;
		this->_outHeadersValid = rhs._outHeadersValid;
		this->_outTimestamp = rhs._outTimestamp;
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

long long	CGI::getContentLength(void) const
{
	return (this->_contentLength);
}

std::string	CGI::getContentType(void) const
{
	return (this->_contentType);
}

Location	&CGI::getLocation(void)
{
	return (*(this->_loc));
}

struct timeval	CGI::getOutTimestamp(void) const
{
	return (this->_outTimestamp);
}

int	CGI::getStatus(void) const
{
	return (this->_status);
}

void	CGI::setCGIContentLength(long long length)
{
	this->_contentLength = length;
}

void	CGI::setCGIContentType(std::string type)
{
	this->_contentType = type;
}

void	CGI::addBytesSent(int bytes)
{
	this->_bytesSent += bytes;
}

void	CGI::setOutTimestamp(struct timeval tv)
{
	this->_outTimestamp = tv;
}


/*******************************************************************************
*						INIT
*******************************************************************************/
void	freeEnv(char **env)
{
	if (!env || !(*env))
		return ;

	for (int i = 0; env[i]; i++)
		delete [] env[i];

	delete[] env;
}

void	CGI::_setupEnvPOST(std::vector<std::string> env, char ***childEnv, Client &client)
{
	int	childEnvSize = env.size() + 2; //for CONTENT_LENGTH var and NULL
	if (client.getRequest().getCookies() != "")
		childEnvSize++;
	*childEnv = new char *[childEnvSize];
	for (size_t i = 0; i < env.size(); i++)
	{
		(*childEnv)[i] = new char[env[i].length() + 1];
		std::strcpy((*childEnv)[i], env[i].c_str());
	}
	if (client.getRequest().getCookies() != "")
	{
		std::string cookies("COOKIE=");
		cookies += client.getRequest().getCookies();
		(*childEnv)[childEnvSize - 3] = new char[cookies.length() + 1];
		std::strcpy((*childEnv)[childEnvSize - 3], cookies.c_str());
	}
	if (client.getRequest().getContentLength() == 0)
		throw (std::runtime_error("bad request"));
	std::string	contentLength("CONTENT_LENGTH=");
	std::stringstream sstr;
	sstr << client.getRequest().getContentLength();
	contentLength += sstr.str();
	(*childEnv)[childEnvSize - 2] = new char[contentLength.length() + 1];
	std::strcpy((*childEnv)[childEnvSize - 2], contentLength.c_str());
	(*childEnv)[childEnvSize - 1] = NULL;
}

void	CGI::_setupEnvGET(std::string queryString, std::vector<std::string> env, char ***childEnv, Client &client)
{
	int	childEnvSize = env.size() + 1; //for CONTENT_LENGTH var and NULL // why CONTENT_LENGTH
	if (client.getRequest().getCookies() != "")
		childEnvSize++;
	if (!queryString.empty())
		childEnvSize++;
	*childEnv = new char *[childEnvSize];
	for (size_t i = 0; i < env.size(); i++)
	{
		(*childEnv)[i] = new char[env[i].length() + 1];
		std::strcpy((*childEnv)[i], env[i].c_str());
	}
	if (client.getRequest().getCookies() != "")
	{
		int	idx = queryString.empty() ? childEnvSize - 2 : childEnvSize - 3;
		std::string cookies("COOKIE=");
		cookies += client.getRequest().getCookies();
		(*childEnv)[idx] = new char[cookies.length() + 1];
		std::strcpy((*childEnv)[idx], cookies.c_str());
	}
	// if (client.getRequest().getContentLength() == 0) //get do not have contentLenght?
	// 	throw (std::runtime_error("bad request"));
	if (!queryString.empty())
	{
		std::string	qString("QUERY_STRING=");
		qString += queryString;
		(*childEnv)[childEnvSize - 2] = new char[qString.length() + 1];
		std::strcpy((*childEnv)[childEnvSize - 2], qString.c_str());
	}
	(*childEnv)[childEnvSize - 1] = NULL;
}

std::string	getProgPath(std::string scriptPath)
{
	std::string ext = scriptPath.substr(scriptPath.find_last_of('.'));
	if (ext == ".py")
		return "/usr/bin/python3";
	if (ext == ".php")
		return "/usr/bin/php";
	throw(std::runtime_error(".py or .php only"));
}

void	CGI::_createChildProcess(int *inPipe, int *outPipe, char **childEnv)
{
		dup2(inPipe[0], STDIN_FILENO);
		dup2(outPipe[1], STDOUT_FILENO);
		close(inPipe[1]);
		close(outPipe[0]);

		std::string	progPath = getProgPath(this->_scriptPath);

		char		*path = const_cast<char*>(progPath.c_str());
		char		*argv[] = {	path,
								const_cast<char*>(this->_scriptPath.c_str()),
								NULL};
		if (this->_cgiEnv.empty() || execve(path, argv, childEnv) == -1)
		{
			freeEnv(childEnv);
			close(inPipe[0]);
			close(outPipe[1]);
			exit(1);
		}
}



/*******************************************************************************
*						READ / WRITE / CLOSE
*******************************************************************************/

void	CGI::closeCgi(int epollFD)
{

	int	status;
	waitpid(this->_pid, &status, WNOHANG); //non blocking 
	if (WIFEXITED(status) && WEXITSTATUS(status) != 0)
	{
		// script error,  return appropriate code
	}

	if (this->_writeFd != -1)
	{
		close(this->_inFileFd);
		epoll_ctl(epollFD, EPOLL_CTL_DEL, this->_writeFd, NULL);
		close(this->_writeFd);
		// _cgiMap.erase(cgi.writeFd); //handled in handleCgiOutput
		this->_writeFd = -1;
	}
	epoll_ctl(epollFD, EPOLL_CTL_DEL, this->_readFd, NULL);
	close(this->_readFd);
	// _cgiMap.erase(readFd); //handle in handleCgiOutput	
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

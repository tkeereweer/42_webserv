#include "../include/CGI.hpp"
#include "../include/Webserv.hpp"
#include <sstream>


CGI::CGI(void) {}

void	freeEnv(char **env);

//constructor for get method CGI with query_string argument
CGI::CGI(std::string queryString, std::vector<std::string> env, Client &client, Location *loc, std::string scriptPath):
	_loc(loc),
	_clientFd(client.getFd()),
	_writeFd(-1),
	_inFileFd(-1),
    _bytesWritten(0),
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

	int outPipe[2];
	if (pipe(outPipe) == -1)
		throw (std::runtime_error(std::strerror(errno)));

	//setup environment variables
	char	**childEnv;
	_setupEnvGET(queryString, env, &childEnv, client);

		std::string	progPath = _getProgPath(this->_scriptPath);
		std::cout<<"PROGPATH:"<<progPath<<std::endl;

	//create child
	this->_pid = fork();
	if (this->_pid == -1)
		throw (std::runtime_error(std::strerror(errno)));
	if (this->_pid == 0)
		_createChildProcess(NULL, outPipe, childEnv);
	close(outPipe[1]);
	freeEnv(childEnv);
	this->_readFd = outPipe[0];
	this->_outTimestamp.tv_usec = 0;
	this->_outTimestamp.tv_sec = 0;
}

//constructor for post method CGI
CGI::CGI(std::vector<std::string> env, Client &client, Location *loc, std::string scriptPath):
	_loc(loc),
	_clientFd(client.getFd()),
    _bytesWritten(0),
    _bytesSent(0),
    _scriptPath(scriptPath),
    _cgiEnv(env),
	_contentLength(-1),
	_status(-1),
	_outComplete(false),
	_outHeadersValid(false)
{
	this->_inFileFd = open(client.getRequest().getBodyFilename().c_str(), O_RDONLY);
	if (this->_inFileFd == -1)
		throw (std::runtime_error(std::strerror(errno)));
		// TODO handle error here, already done with throw??
	//add what's below when switching back to relative paths
	// std::string dot(".");
	// this->_scriptPath.insert(this->_scriptPath.begin(), dot.begin(), dot.end());
	int	inPipe[2];
	int outPipe[2];
	if (pipe(inPipe) == -1)
	{
		close(this->_inFileFd);
		throw (std::runtime_error(std::strerror(errno)));
	}
	if (pipe(outPipe) == -1)
	{
		close(inPipe[0]);
		close(inPipe[1]);
		close(this->_inFileFd);
		throw (std::runtime_error(std::strerror(errno)));
	}

	//setup environment variables
	char	**childEnv;
	_setupEnvPOST(env, &childEnv, client);

	//create child
	this->_pid = fork();
	if (this->_pid == -1)
	{
		close(inPipe[0]);
		close(inPipe[1]);
		close(outPipe[0]);
		close(outPipe[1]);
		close(this->_inFileFd);
		throw (std::runtime_error(std::strerror(errno)));
	}
	if (this->_pid == 0)
		_createChildProcess(inPipe, outPipe, childEnv);
	close(inPipe[0]);
	close(outPipe[1]);
	freeEnv(childEnv);

	this->_readFd = outPipe[0];
	this->_writeFd = inPipe[1]; //only for post method

	this->_outTimestamp.tv_usec = 0;
	this->_outTimestamp.tv_sec = 0;
}

CGI::CGI(CGI const &src)
{
	*this = src;
}

CGI::~CGI(void)
{
	// waitpid(this->_pid, NULL, WNOHANG); //WNOHANG: returns immediately if no child has exited
}

CGI	&CGI::operator=(CGI const &rhs)
{
	if (this != &rhs)
	{
		this->_loc = rhs._loc;
		this->_clientFd = rhs._clientFd;
		this->_pid = rhs._pid;
		this->_writeFd = rhs._writeFd;
		this->_readFd = rhs._readFd;
		this->_inFileFd = rhs._inFileFd;
		this->_outBuff = rhs._outBuff;
		this->_bytesSent = rhs._bytesSent;
        this->_bytesWritten = rhs._bytesWritten;
		this->_cgiEnv = rhs._cgiEnv;
		this->_queryString = rhs._queryString;
		this->_contentLength = rhs._contentLength;
		this->_contentType = rhs._contentType;
        this->_setCookie = rhs._setCookie;
		this->_outComplete = rhs._outComplete;
		this->_outHeadersValid = rhs._outHeadersValid;
		this->_outTimestamp = rhs._outTimestamp;
		this->_status = rhs._status;
	}
	return (*this);
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
	if (!queryString.empty())
	{
		std::string	qString("QUERY_STRING=");
		qString += queryString;
		(*childEnv)[childEnvSize - 2] = new char[qString.length() + 1];
		std::strcpy((*childEnv)[childEnvSize - 2], qString.c_str());
	}
	(*childEnv)[childEnvSize - 1] = NULL;
}

std::string	CGI::_getProgPath(std::string& scriptPath)
{
	std::string ext = scriptPath.substr(scriptPath.find_last_of('.'));
	std::string prog;
	if (ext == ".py")
		return (_pathfinder("python3"));
		// return "/usr/bin/python3";
	else if (ext == ".php")
		return (_pathfinder("php"));
		// return "/usr/bin/php";
	else
		throw(std::runtime_error(".py or .php only"));
	
	
}

std::string CGI::_pathfinder(std::string prog)
{
	std::vector<std::string>::iterator	it;

	for (it = _cgiEnv.begin(); it != _cgiEnv.end(); it++)
	{
		if (it->substr(0, 5) == "PATH=")
			break;
	}

	if (it == _cgiEnv.end())
		throw(std::runtime_error("no path in env"));

	std::istringstream	ss(it->substr(5));
	std::string			token;
	while (std::getline(ss, token, ':'))
	{
		std::string		path = token + "/" + prog;
		if (access(path.c_str(), F_OK) == 0)
			return (path);
	}
	throw(std::runtime_error("path not found for: " + prog));
}

void	CGI::_createChildProcess(int *inPipe, int *outPipe, char **childEnv)
{
	if (this->_inFileFd != -1)
		close(this->_inFileFd);
	if (inPipe != NULL)
	{
		dup2(inPipe[0], STDIN_FILENO);
		close(inPipe[1]);
	}
	dup2(outPipe[1], STDOUT_FILENO);
	close(outPipe[0]);

	std::string	progPath = _getProgPath(this->_scriptPath);

	char		*path = const_cast<char*>(progPath.c_str());
	char		*argv[] = {	path,
							const_cast<char*>(this->_scriptPath.c_str()),
							NULL};
	if (this->_cgiEnv.empty() || execve(path, argv, childEnv) == -1)
	{
		freeEnv(childEnv);
		if (inPipe != NULL)
			close(inPipe[0]);
		close(outPipe[1]);
		exit(1);
	}
}



/*******************************************************************************
*						READ / WRITE / CLOSE
*******************************************************************************/

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

ssize_t		CGI::getBytesWritten(void) const
{
	return (this->_bytesWritten);
}

long long	CGI::getContentLength(void) const
{
	return (this->_contentLength);
}

std::string	CGI::getContentType(void) const
{
	return (this->_contentType);
}

std::string CGI::getSetCookie(void) const
{
    return (this->_setCookie);
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

int	CGI::getInFileFD(void) const
{
	return (this->_inFileFd);
}

void	CGI::setCGIContentLength(long long length)
{
	this->_contentLength = length;
}

void	CGI::setCGIContentType(std::string type)
{
	this->_contentType = type;
}

void	CGI::addBytesSent(ssize_t bytes)
{
	this->_bytesSent += bytes;
}

void	CGI::addBytesWritten(ssize_t bytes)
{
	this->_bytesWritten += bytes;
}

void    CGI::setOutTimestamp(struct timeval tv)
{
    this->_outTimestamp = tv;
}

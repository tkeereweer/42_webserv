#include "CGI.hpp"
#include "Webserv.hpp"
#include <sstream>


CGI::CGI(void) {}

void	freeEnv(char **env);

//constructor for GET method CGI with query_string argument
CGI::CGI(std::string queryString, std::vector<std::string> env, Client &client, Location *loc, std::string scriptPath):
	_loc(loc),
	_clientFd(client.getFd()),
	_errorFD(-1),
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
	_outHeadersValid(false),
    _startTimestamp(0)
{
	//don't build input pipe but append queryString as query_string=<value here> to end of servEnv.

	int errorPipe[2];
	int outPipe[2];
	if (pipe(errorPipe) == -1)
		throw (std::runtime_error(std::strerror(errno)));
	fcntl(errorPipe[1], F_SETFD, FD_CLOEXEC);
	if (pipe(outPipe) == -1)
	{
		close(errorPipe[0]);
		close(errorPipe[1]);
		throw (std::runtime_error(std::strerror(errno)));
	}

	//setup environment variables
	char	**childEnv;
	_setupEnvGET(queryString, env, &childEnv, client);

	std::string	progPath = _getProgPath(this->_scriptPath);
	std::cout<<"PROGPATH:"<<progPath<<std::endl;

	//create child
	this->_pid = fork();
	if (this->_pid == -1)
	{
		close(errorPipe[0]);
		close(errorPipe[1]);
		close(outPipe[0]);
		close(outPipe[1]);
		throw (std::runtime_error(std::strerror(errno)));
	}
	//take creation timestamp
    time(&this->_startTimestamp);
	if (this->_pid == 0)
		_createChildProcess(NULL, outPipe, childEnv, errorPipe);

	close(errorPipe[1]);
	close(outPipe[1]);
	freeEnv(childEnv);
	this->_errorFD = errorPipe[0];
	this->_readFd = outPipe[0];
}

//constructor for POST method CGI
CGI::CGI(std::vector<std::string> env, Client &client, Location *loc, std::string scriptPath):
	_loc(loc),
	_clientFd(client.getFd()),
	_errorFD(-1),
	_writeFd(-1),
	_inFileFd(-1),
	_bytesWritten(0),
	_bytesSent(0),
	_scriptPath(scriptPath),
	_cgiEnv(env),
	_contentLength(-1),
	_status(-1),
	_outComplete(false),
	_outHeadersValid(false),
    _startTimestamp(0)
{
	//open body file
	this->_inFileFd = open(client.getRequest().getBodyFilename().c_str(), O_RDONLY);
	if (this->_inFileFd == -1)
		throw (std::runtime_error(std::strerror(errno)));

	int errorPipe[2];
	int	inPipe[2];
	int outPipe[2];
	if (pipe(errorPipe) == -1)
	{
		close(this->_inFileFd);
		throw (std::runtime_error(std::strerror(errno)));
	}
	fcntl(errorPipe[1], F_SETFD, FD_CLOEXEC);
	if (pipe(inPipe) == -1)
	{
		close(this->_inFileFd);
		close(errorPipe[0]);
		close(errorPipe[1]);
		throw (std::runtime_error(std::strerror(errno)));
	}
	if (pipe(outPipe) == -1)
	{
		close(errorPipe[0]);
		close(errorPipe[1]);
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
		close(errorPipe[0]);
		close(errorPipe[1]);
		close(this->_inFileFd);
		throw (std::runtime_error(std::strerror(errno)));
	}
    time(&this->_startTimestamp);
	if (this->_pid == 0)
		_createChildProcess(inPipe, outPipe, childEnv, errorPipe);

	close(errorPipe[1]);
	close(inPipe[0]);
	close(outPipe[1]);
	freeEnv(childEnv);
	this->_errorFD = errorPipe[0];
	this->_readFd = outPipe[0];
	this->_writeFd = inPipe[1]; //only for post method
}

CGI::CGI(CGI const &src)
{
	*this = src;
}

CGI::~CGI(void){}

CGI	&CGI::operator=(CGI const &rhs)
{
	if (this != &rhs)
	{
		this->_loc = rhs._loc;
		this->_clientFd = rhs._clientFd;
		this->_pid = rhs._pid;
		this->_errorFD = rhs._errorFD;
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
        this->_startTimestamp = rhs._startTimestamp;
		this->_status = rhs._status;
	}
	return (*this);
}


/*******************************************************************************
*						GET / SET /OTHER
*******************************************************************************/

//getters
int			&CGI::getClientFD(void)
{
	return (this->_clientFd);
}

int 		CGI::getPID(void) const
{
	return (this->_pid);
}

int			&CGI::getErrorFD(void)
{
	return (this->_errorFD);
}

int			&CGI::getWriteFD(void)
{
	return (this->_writeFd);
}

int			&CGI::getReadFD(void)
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

std::time_t	CGI::getStartTimestamp(void) const
{
	return (this->_startTimestamp);
}

int	CGI::getStatus(void) const
{
	return (this->_status);
}

int	&CGI::getInFileFD(void)
{
	return (this->_inFileFd);
}

std::string	CGI::getLocationHeader(void) const
{
	return (this->_locationHeader);
}

//set
void	CGI::setCGIContentLength(long long length)
{
	this->_contentLength = length;
}

void	CGI::setCGIContentType(std::string type)
{
	this->_contentType = type;
}

//other
void	CGI::addBytesSent(ssize_t bytes)
{
	this->_bytesSent += bytes;
}

void	CGI::addBytesWritten(ssize_t bytes)
{
	this->_bytesWritten += bytes;
}


void    CGI::resetStartTimestamp(void)
{
    this->_startTimestamp = 0;
}

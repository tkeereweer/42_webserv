
#include "../include/Webserv.hpp"
#include <sys/wait.h>
#include <fcntl.h>
#include <sys/epoll.h>
#include <cstring>
#include <dirent.h>

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

	t_cgi	cgi = populateCgiStruct(clientFd, pid, inPipe, outPipe, _cgiMap);

	addCgiToEpoll(cgi);
}


void	Webserv::createPipes(int *inPipe, int *outPipe)
{
	if (pipe(inPipe) == -1)
		throw (std::runtime_error(std::strerror(errno)));
	if (pipe(outPipe) == -1)
	{
		close(inPipe[0]);
		close(inPipe[1]);
		throw (std::runtime_error(std::strerror(errno)));
	}
}


void	Webserv::childProcessCgi(int *inPipe, int *outPipe, int clientFd)
{
		dup2(inPipe[0], STDIN_FILENO);
		dup2(outPipe[1], STDOUT_FILENO);
		close(inPipe[1]);
		close(outPipe[0]);

		std::string	scriptPath = getScriptPath(clientFd);
		std::string	progPath = getProgPath(scriptPath);

		char		*path = const_cast<char*>(progPath.c_str());
		char		*argv[] = {	path,
								const_cast<char*>(scriptPath.c_str()),
								NULL};
		char		**env = setupEnv(clientFd);

		if (env == NULL || execve(path, argv, env) == -1)
		{
			freeEnv(env);
			close(inPipe[0]);
			close(outPipe[1]);
			exit(1);
		}
}


t_cgi	Webserv::populateCgiStruct(int clientFd, pid_t pid, int *inPipe, int *outPipe, std::map<int, t_cgi>& _cgiMap)
{
	Client	&client = _clientMap[clientFd].client;
	t_cgi	cgi;

	cgi.clientFd = clientFd;
	cgi.pid = pid;
	cgi.bytesSent = 0;
	cgi.readFd = outPipe[0];
	_createTempFile(cgi);
	cgi.outFileFd = open(cgi.outFile.c_str(), O_WRONLY);
	if (cgi.outFileFd == -1)
		throw(std::runtime_error(std::strerror(errno)));
	if (client.getRequest().getMethod() == POST)
	{
		cgi.writeFd = inPipe[1];
		cgi.inFileFd = open(client.getRequest().getBodyFilename().c_str(), O_RDONLY);
		if (cgi.inFileFd == -1)
			throw (std::runtime_error(std::strerror(errno)));
		_cgiMap[cgi.writeFd] = cgi;
	}
	else
	{
		close(inPipe[1]);
		cgi.writeFd = -1;
		cgi.inFileFd = -1;
	}
	_cgiMap[outPipe[0]] = cgi;

	return (cgi);
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


/*******************************************************************************
*						CHILD HELPERS
*******************************************************************************/

std::string	Webserv::getScriptPath(int clientFd)
{
	return ("." + _clientMap[clientFd].client.getRequest().getURI());
}


std::string	Webserv::getProgPath(std::string scriptPath)
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

void	Webserv::handleCgiInput(int writeFd)
{
	t_cgi	&cgi = _cgiMap[writeFd];
	Request	&request = _clientMap[cgi.clientFd].client.getRequest();
	char	buffer[1024];

	ssize_t bytesRead = read(cgi.inFileFd, buffer, sizeof(buffer) - 1);

	if (bytesRead > 0)
	{
		buffer[bytesRead] = 0;
		ssize_t bytesSentNow = write(writeFd, buffer, bytesRead);
		cgi.bytesSent += bytesSentNow;
		if (cgi.bytesSent >= request.getContentLength())
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

void    Webserv::_createTempFile(t_cgi& cgi)
{
	if (cgi.outFile != "")
		return ;
	
	DIR *tmp = opendir("/tmp");
	if (tmp == NULL)
		throw(std::runtime_error(strerror(errno)));

	struct dirent *name = readdir(tmp);
	if (!name && errno != 0)
		throw(std::runtime_error(strerror(errno)));
	while (name && strncmp(name->d_name, "wbsrv_cgi_", strlen("wbsrv_cgi_")))
		name = readdir(tmp);
	if (errno != 0)
		throw(std::runtime_error(strerror(errno)));
	//no temp file, create from 0
	if (!name)
	{
		std::ofstream file("/tmp/wbsrv_cgi_0");
		if (!file)
			throw(std::runtime_error("can't open file"));
        cgi.outFile = "/tmp/wbsrv_cgi_0";
		return ;
	}

	//handle next available file logic
	return (_createNextAvailableFile(name, tmp, cgi));
}


void	Webserv::_createNextAvailableFile(struct dirent *name, DIR *tmp, t_cgi& cgi)
{
	std::string lastFile(name->d_name);
	int num = atoi(std::string(&lastFile[lastFile.find_last_of("_") + 1]).c_str()); //first num found
	int next_num = 0;
	std::ostringstream numWrite;
	std::string	nameToFind = "wbsrv_cgi_";

	//exit loop if no file found or intmax number of temp files
	while (name && next_num < std::numeric_limits<int>::max())
	{
		nameToFind = "wbsrv_cgi_";
		numWrite << next_num;
		nameToFind += numWrite.str();
		//finds file with nex number
		while (name && strncmp(name->d_name, nameToFind.c_str(), strlen(nameToFind.c_str())))
			name = readdir(tmp);
		next_num++;
		numWrite.str("");
	}
	if (errno != 0)
		throw(std::runtime_error(strerror(errno)));
	if (num == std::numeric_limits<int>::max())
		throw(std::runtime_error("out of room for temp files"));
	
	//create available file found
	std::string	filename("/tmp/");
	filename += nameToFind;
	std::ofstream file(filename.c_str());
	if (!file)
			throw(std::runtime_error("can't open file"));
    cgi.outFile = filename;
	return ;	
}

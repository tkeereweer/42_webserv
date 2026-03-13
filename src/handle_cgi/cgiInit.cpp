#include "CGI.hpp"
#include "Webserv.hpp"
#include <sstream>


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
	{
		//close pipes here !
		throw (std::runtime_error("bad request"));
	}
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

void	CGI::_createChildProcess(int *inPipe, int *outPipe, char **childEnv, int *errorPipe)
{
	close(errorPipe[0]);
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
	errno = 0;
	if (this->_cgiEnv.empty() || execve(path, argv, childEnv) == -1)
	{
		write(errorPipe[1], &errno, sizeof(errno));
		freeEnv(childEnv);
		if (inPipe != NULL)
			close(inPipe[0]);
		close(outPipe[1]);
		close(errorPipe[1]);
		exit(127);
	}
}

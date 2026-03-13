#include "Webserv.hpp"

volatile sig_atomic_t	g_signum = 0;
int						g_sigPipe[2];

std::string	findExeRoot(void);

std::string exeRoot = findExeRoot();

std::string	findExeRoot(void)
{
	char	path[PATH_MAX];
	ssize_t	length = readlink("/proc/self/exe", path, sizeof(path)); //finds path of executable;
	if (length == -1)
		return ("");
	std::string	execPath(path, length);
	std::string::iterator end = execPath.begin() + execPath.find_last_of("/") + 1;
	return (std::string(execPath.begin(), end));
}

//takes path in root like /data/www/... and build the absolute path compared to actual execLocation
std::string	resolvePath(std::string relPath)
{
	if (relPath[0] == '/')
		relPath = &relPath[1];
	std::string	resultingPath = exeRoot + relPath;
	return (resultingPath);
}

void	signalHandler(int signum)
{
	if (signum == SIGINT)
	{
		g_signum = SIGINT;
		write(g_sigPipe[1], "a", 1);
	}
	return ;
}

int	main(int argc, char *argv[], char **envp)
{
	Webserv		webserv(envp);
    std::string output = resolvePath("config/default.conf");
    const char *config_file = output.c_str();

	if (argc > 1)
		config_file = argv[1];
	try
	{
		webserv.getConfig(config_file);
		std::cout << "Parsed config file: " << std::endl;
		// std::cout << webserv << std::endl;
        if (pipe(g_sigPipe) == -1)
		    return (1);
        signal(SIGINT, &signalHandler);
		webserv.openSockets();
		webserv.launchServer();
		close(g_sigPipe[0]);
		close(g_sigPipe[1]);
	}
	catch(const std::exception& e)
	{
		std::cerr << e.what() << std::endl;
		if (webserv.getEpollFd() != -1)
			close(webserv.getEpollFd());
		for (std::map<int, Server*>::iterator it = webserv.getServerMap().begin(); it != webserv.getServerMap().end(); it++)
			close(it->first);
		close(g_sigPipe[0]);
		close(g_sigPipe[1]);
        return (1);
	}
	return (0);
}

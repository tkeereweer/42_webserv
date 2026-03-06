#include "../include/Webserv.hpp"

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
    std::cout << "resultingPath: " << resultingPath << std::endl;
	return (resultingPath);
}

int	main(int argc, char *argv[], char **envp)
{
	Webserv		webserv(envp);
	// const char	*config_file = "./config/default.conf";
    std::string output = resolvePath("config/default.conf");
    const char *config_file = output.c_str();

	if (argc > 1)
		config_file = argv[1];
	try
	{
		webserv.getConfig(config_file);
		std::cout << "Parsed config file: " << std::endl;
		std::cout << webserv << std::endl;
		webserv.openSockets();
		webserv.launchServer();
	}
	catch(const std::exception& e)
	{
		std::cerr << e.what() << std::endl;
		for (std::map<int, Server*>::iterator it = webserv.getServerMap().begin(); it != webserv.getServerMap().end(); it++)
			close(it->first);
	}
	return (0);
}

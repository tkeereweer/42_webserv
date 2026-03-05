#include "../include/Webserv.hpp"

int	main(int argc, char *argv[], char **envp)
{
	Webserv		webserv(envp);
	// const char	*config_file = "./config/default.conf";
    const char *config_file = "/home/sravizza/42/rank05/webserv_perso/config/default.conf";

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

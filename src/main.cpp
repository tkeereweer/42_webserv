#include "../include/Webserv.hpp"
#include <iostream>

int	main(int argc, char *argv[])
{
	Webserv		webserv;
	const char	*config_file = "./config/default.conf";

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
	}
	
	// Server		serverA;
	// t_socket	socketA;

	// socketA.port = "8080";
	// socketA.ipAddr = "127.0.0.1";
	// serverA.addSocket(socketA);
	// webserv.addServer(serverA);
	// webserv.openSockets();
	// webserv.launchServer();
	return (0);
}
#include "../include/Webserv.hpp"
#include <iostream>

int	main(void)
{
	Webserv		webserv;
	std::string	file = "test.conf";

	try
	{
		webserv.getConfig(file.c_str());
		std::cout << "Parsed config file: " << std::endl;
		std::cout << webserv << std::endl;
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
#include "../include/Webserv.hpp"

int	main(void)
{
	Webserv		webserv;
	Server		serverA;
	t_socket	socketA;

	socketA.port = "8080";
	socketA.ipAddr = "127.0.0.1";
	serverA.addSocket(socketA);
	webserv.addServer(serverA);
	webserv.openSockets();
	webserv.launchServer();
	return (0);
}
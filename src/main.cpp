#include "../include/Webserv.hpp"

int	main(void)
{
	Webserv	webserv(1);
	Server	serverA;
	Socket	socketA("80", "0.0.0.0");

	serverA.setSocket(socketA, 0);
	webserv.setServer(serverA, 0);
	webserv.openSockets();
	webserv.launchServer();
	return (0);
}
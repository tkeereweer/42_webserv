#include "Server.hpp"

std::string	resolvePath(std::string relPath);

/*******************************************************************************
*						CGI EPOLL
*******************************************************************************/

void	Server::addCgiToEpoll(CGI &cgi, int epollFD) const
{
	epoll_event	ev;
	ev.events = EPOLLIN;
	ev.data.fd = cgi.getErrorFD();
	fcntl(cgi.getErrorFD(), F_SETFL, O_NONBLOCK);
	if (epoll_ctl(epollFD, EPOLL_CTL_ADD, cgi.getErrorFD(), &ev) == -1)
	{
		ev.events = EPOLLOUT;
		ev.data.fd = cgi.getClientFD();
		if (epoll_ctl(epollFD, EPOLL_CTL_ADD, cgi.getClientFD(), &ev) == -1)
            throw (std::runtime_error("epoll ctl fail"));
	}
}

/*******************************************************************************
*						GET
*******************************************************************************/

void	Server::_handleGET(Client &client, Location &loc, std::string path, int epollFD)
{
	try
	{
		Request	&req = client.getRequest();
        errno = 0;
		if (access(path.c_str(), F_OK | R_OK) == -1)
		{
			std::cout << "access errno: " << strerror(errno) << std::endl;
			std::cout << "path: " << path << std::endl << "path.c_str(): " << path.c_str() << std::endl;
            if (errno == ENOENT)
			    return (client.getResponse().buildErrorResponse(404, this, &loc));
            else
                return (client.getResponse().buildErrorResponse(403, this, &loc));
		}
		//handle GET Cgi
		if (req.getURI().find(".py") != std::string::npos || req.getURI().find(".php") != std::string::npos) //.php or any other handled cgi
			return (client.getResponse().buildGetCGIResponse(client, &loc, epollFD, *this, path)); //needs full path in there
		return (client.getResponse().buildRouteResponse(path, this, &loc));
	}
	catch (std::exception const &e)
	{
		std::cerr << e.what() << std::endl;
		return (client.getResponse().buildErrorResponse(500, this, &loc));
	}
}


/*******************************************************************************
*						POST
*******************************************************************************/

void	Server::_handlePOST(Location &loc, Client &client, std::string path, int epollFD)
{
	try
	{
		Request	&req = client.getRequest();
		std::cout << "path:"<< req.getURI() << std::endl;
		//handle POST Cgi
		if (req.getURI().find(".py") != std::string::npos || req.getURI().find(".php") != std::string::npos) //.php or any other handled cgi
			return (client.getResponse().buildPostCgiResponse(client, &loc, epollFD, *this, path));
		if (loc.getUploadStore().empty())
			return (client.getResponse().buildErrorResponse(403, this, &loc));
		return (_uploadFile(loc, client));
	}
	catch(const std::exception& e)
	{
		return (client.getResponse().buildErrorResponse(500, this, &loc));
        //add client to epoll
	}
}


void	Server::_uploadFile(Location &loc, Client &client)
{
	Request			&req = client.getRequest();
	std::ifstream	ifs;
	std::string		uploadPath = resolvePath("");
	std::string		fileName;
	std::ofstream	ofs;
	char			buffer[4056];
	std::streamsize	read;

	uploadPath.append(loc.getUploadStore());
	fileName  = std::string(req.getURI(), req.getURI().find_last_of('/') + 1);
	if (fileName.empty())
		return (client.getResponse().buildErrorResponse(400, this, &loc));
	uploadPath += "/" + fileName;
	ifs.open(req.getBodyFilename().c_str(), std::ios_base::in | std::ios_base::binary);
	ofs.open(uploadPath.c_str(), std::ios_base::out | std::ios_base::binary);
	if (!ifs.is_open() || !ofs.is_open())
		return (client.getResponse().buildErrorResponse(500, this, &loc));
	while (!ifs.eof() && !ifs.bad())
	{
		ifs.read(buffer, sizeof(buffer));
		read = ifs.gcount();
		ofs.write(buffer, read);
	}
	client.getResponse().buildPostResponse(uploadPath);
}


/*******************************************************************************
*						DELETE
*******************************************************************************/

void	Server::_handleDELETE(Location &loc, Client &client, std::string& path)
{
	try
	{
		return (client.getResponse().buildDelResponse(client, path, this, &loc));
	}
	catch(const std::exception& e)
	{
		std::cerr << e.what() << std::endl;
		return (client.getResponse().buildErrorResponse(500, this, &loc));
	}
	


}

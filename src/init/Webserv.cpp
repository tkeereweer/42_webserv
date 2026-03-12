#include "Webserv.hpp"

std::string	resolvePath(std::string relPath);

/*******************************************************************************
*						CTOR/DTOR
*******************************************************************************/

Webserv::Webserv(void): _epollFd(-1)
{}

Webserv::Webserv(char **envp): _epollFd(-1)
{
	for (int i = 0; envp[i] != NULL; i++)
	{
		this->_parentEnv.push_back(std::string(envp[i]));
	}
}

Webserv::Webserv(Webserv const &src):
	_servers(src._servers),
	_serverMap(src._serverMap),
	_clientMap(src._clientMap),
	_epollFd(src._epollFd),
	_parentEnv(src._parentEnv)
{}

Webserv	&Webserv::operator=(Webserv const &rhs)
{
	if (this != &rhs)
	{
		this->_servers = rhs._servers;
		this->_serverMap = rhs._serverMap;
		this->_clientMap = rhs._clientMap;
		this->_epollFd = rhs._epollFd; // close old epollFd ??
		this->_parentEnv = rhs._parentEnv;
	}
	return (*this);
}

Webserv::~Webserv(void) {}

std::ostream	&operator<<(std::ostream &o, Webserv &input)
{
	for (unsigned int i = 0; i < input.getServers().size(); i++)
	{
		o << "server {" << std::endl;
		o << "\tserver_name " << input.getServers()[i].getName() << ";" << std::endl;
		o << "\troot " << input.getServers()[i].getRoot() << ";" << std::endl;
		o << "\tlimit_except GET:" << input.getServers()[i].getAcceptGET() << " POST:" << input.getServers()[i].getAcceptPOST() << " DELETE:" << input.getServers()[i].getAcceptDELETE() << ";" << std::endl;
		o << "\tautoindex " << input.getServers()[i].getAutoIndex() << ";" << std::endl;
		o << "\tindex " << input.getServers()[i].getIndex() << ";" << std::endl;
		o << "\tclient_max_body_size " << input.getServers()[i].getMaxBody() << ";" << std::endl;
		for (unsigned int j = 0; j < input.getServers()[i].getSockets().size(); j++)
			o << "\tlisten " << input.getServers()[i].getSockets()[j].ipAddr << "/" << input.getServers()[i].getSockets()[j].port << ";" << std::endl;
		for (std::map<int, std::string>::iterator it = input.getServers()[i].getErrorPages().begin(); it != input.getServers()[i].getErrorPages().end(); ++it)
				o << "\terror_page " << it->first << " " << it->second << ";" << std::endl;
			o << "\treturn " << input.getServers()[i].getRedir().first << " " << input.getServers()[i].getRedir().second << ";" << std::endl;
		o << "\tcgi_max_output_size " << input.getServers()[i].getMaxCGIOutput() << ";" << std::endl;
		for (unsigned int j = 0; j < input.getServers()[i].getLocations().size(); j++)
		{
			o << "\tlocation " << input.getServers()[i].getLocations()[j].getPath() << " {" << std::endl;
			o << "\t\troot " << input.getServers()[i].getLocations()[j].getRoot() << ";" << std::endl;
			o << "\t\tlimit_except GET:" << input.getServers()[i].getLocations()[j].getAcceptGET() << " POST:" << input.getServers()[i].getLocations()[j].getAcceptPOST() << " DELETE:" << input.getServers()[i].getLocations()[j].getAcceptDELETE() << ";" << std::endl;
			o << "\t\tautoindex " << input.getServers()[i].getLocations()[j].getAutoIndex() << ";" << std::endl;
			o << "\t\tindex " << input.getServers()[i].getLocations()[j].getIndex() << ";" << std::endl;
			o << "\t\tclient_max_body_size " << input.getServers()[i].getLocations()[j].getMaxBody() << ";" << std::endl;
			o << "\t\tupload_store " << input.getServers()[i].getLocations()[j].getUploadStore() << ";" << std::endl;
			for (std::map<int, std::string>::iterator it = input.getServers()[i].getLocations()[j].getErrorPages().begin(); it != input.getServers()[i].getLocations()[j].getErrorPages().end(); ++it)
				o << "\t\terror_page " << it->first << " " << it->second << ";" << std::endl;
			o << "\t\treturn " << input.getServers()[i].getLocations()[j].getRedir().first << " " << input.getServers()[i].getLocations()[j].getRedir().second << ";" << std::endl;
			o << "\t\tcgi_max_output_size " << input.getServers()[i].getLocations()[j].getMaxCGIOutput() << ";" << std::endl;
			o << "\t}" << std::endl;
		}
		o << "}" << std::endl;
	}
	return (o);
}

/*******************************************************************************
*						GET/SET
*******************************************************************************/

std::vector<Server>	&Webserv::getServers(void)
{
	return (this->_servers);
}

std::map<int, Server*>	&Webserv::getServerMap(void)
{
	return (this->_serverMap);
}

int	Webserv::getEpollFd(void) const
{
	return (this->_epollFd);
}

void	Webserv::addServer(Server server)
{
	this->_servers.push_back(server);
}

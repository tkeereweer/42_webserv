#ifndef SERVER_HPP
# define SERVER_HPP

# include "CGI.hpp"
# include "Config.hpp"
# include "Location.hpp"
# include "libraryHeader.hpp"
# include "Client.hpp"
# include "Request.hpp"
# include "Response.hpp"

typedef struct s_socket
{
	std::string	port;
	std::string	ipAddr;
}	t_socket;


class	Server: public Config
{
	private:
		std::string					_name;
		std::vector<t_socket>		_sockets;
		std::vector<Location>		_locations;
		std::vector<CGI>			_cgiVec; //more logical here as you can choose to have some virtual hosts have access to some CGIs or not.
		std::vector<std::string>	_parentEnv;

		int		matchLocation(std::string URI) const;
		void	_getQueryParams(Request &req);
		void	handleDir(Client &client, Location &loc, std::string dir);
		void	handleGET(Client &client, Location &loc, std::string path, int epollFD);
		void	handlePOST(Location &loc, Client &client, std::string path, int epollFD);
		void	handleDELETE(Location &loc, Client &client, std::string& path);
		void	uploadFile(Location &loc, Client &client);

		Server(void);
	public:
		Server(std::vector<std::string> envp);
		Server(Server const &src);
		Server	&operator=(Server const &rhs);
		virtual ~Server(void);

		std::string					getName(void) const;
		std::vector<t_socket>		&getSockets(void);
		std::vector<Location>		&getLocations(void);

		std::vector<CGI>			&getCgiVec(void);
		std::vector<std::string>	getParentEnv(void) const;

		void	setName(std::string name);
		void	addSocket(t_socket socket);
		void	addLocation(Location location);

		bool		isMethodAllowed(t_method method, Location &loc) const;
		void		dispatchRequest(Client &client, int epollFD);
		void		addCgiToEpoll(CGI &cgi, int epollFD) const;
		std::string	buildPath(std::string URI, Location &loc) const;
};

#endif
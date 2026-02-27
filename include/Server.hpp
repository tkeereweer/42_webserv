#ifndef SERVER_HPP
# define SERVER_HPP

# include <string>
# include <vector>
# include <utility>

# include "Config.hpp"
# include "Location.hpp"
# include "Client.hpp"
# include "CGI.hpp"
#include "Request.hpp"
#include "Response.hpp"

typedef struct s_socket
{
	std::string	port;
	std::string	ipAddr;
}	t_socket;


class	Server: public Config
{
	private:
		std::string				_name;
		std::vector<t_socket>	_sockets;
		std::vector<Location>	_locations;
		std::vector<CGI>		_cgiVec; //more logical here as you can choose to have some virtual hosts have access to some CGIs or not.
		char					**_parentEnv;

		int			matchLocation(std::string URI) const;
		bool		isMethodAllowed(t_method method, Location &loc) const;
		std::string	buildPath(std::string URI, Location &loc) const;
		void		handleError(Client &client, Location &loc, short code) const;
		void		handleDir(Client &client, Location &loc, std::string dir) const;
		void		handleGET(Client &client, Location &loc) const;
		void		handlePOST(Client &client, Location &loc, char **serverEnv, int epollFD);
		void		handleDELETE(Client &client, Location &loc) const;
		void		_addCgiToEpoll(CGI &cgi, int epollFD) const;

		Server(void);
	public:
		Server(char **envp);
		Server(Server const &src);
		Server	&operator=(Server const &rhs);
		virtual ~Server(void);

		std::string					getName(void) const;
		std::vector<t_socket>		&getSockets(void);
		std::vector<Location>		&getLocations(void);

		std::vector<CGI>			&getCgiMap(void);

		void	setName(std::string name);
		void	addSocket(t_socket socket);
		void	addLocation(Location location);

		void	dispatchRequest(Client &client, int epollFD);
};

#endif
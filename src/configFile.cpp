#include "../include/Webserv.hpp"

std::string	Webserv::_openFile(char const *filepath)
{
	std::ifstream	ifs(filepath);
	std::string		line;
	std::string		content;

	if (!ifs.is_open())
		throw(std::runtime_error("Unable to open file"));
	while (std::getline(ifs, line))
		content.append(line);
	return (content);
}

std::list<t_conf_token>	Webserv::_lexConfigFile(std::string content)
{
	std::list<t_conf_token>	out;
	t_conf_token			curr;

	for (std::string::iterator it = content.begin(); it != content.end(); it++)
	{
		if (!isspace(*it) && *it != '{' && *it != '}' && *it != ';')
		{
			curr.type = CONF_WORD;
			curr.value.push_back(*it);
		}
		else
		{
			if (curr.type == CONF_WORD && !curr.value.empty())
			{
				out.push_back(curr);
				curr.value.clear();
			}
			if (*it == '{')
			{
				
				curr.type = OPEN_CURLY;
				curr.value = "{";
				out.push_back(curr);
				curr.value.clear();
			}
			else if (*it == '}')
			{
				
				curr.type = CLOSE_CURLY;
				curr.value = "}";
				out.push_back(curr);
				curr.value.clear();
			}
			else if (*it == ';')
			{
				curr.type = SEMICOLON;
				curr.value = ";";
				out.push_back(curr);
				curr.value.clear();
			}
		}
	}
	return (out);
}

void	Webserv::_printConfTokens(std::list<t_conf_token> lst)
{
	for (std::list<t_conf_token>::iterator it = lst.begin(); it != lst.end(); it++)
	{
		std::cout << it->value << std::endl;
	}
}

void	Webserv::_parseConfTokens(std::list<t_conf_token> &tokens)
{
	std::list<t_conf_token>::iterator	token = tokens.begin();
	std::list<t_conf_token>::iterator	end = tokens.end();
	while (token != end)
	{
		if (token->type == CONF_WORD && token->value == "server")
		{
			Server	server(this->_parentEnv);

			token++;
			if (token != end && token->type == OPEN_CURLY)
			{
				token++;
				_parseServerBlock(token, end, server);
				if (token == end)
					throw(std::runtime_error("Syntax error in config file"));
				if (server.getSockets().size() < 1)
					throw(std::runtime_error("Not enough info for server"));
				this->addServer(server);
			}
			else
				throw(std::runtime_error("Syntax error in config file"));
		}
		else
			throw(std::runtime_error("Config file does not start with a server block"));
		token++;
	}
	
}

/*******************************************************************************
*						CONFIG PARSING
*******************************************************************************/

void	Webserv::_parseLimitExcept(std::list<t_conf_token>::iterator &token, std::list<t_conf_token>::iterator &end, Config &config)
{
	token++;
	if (token == end || token->type != CONF_WORD)
		throw(std::runtime_error("Error in limit_except directive config"));
	config.setAcceptGET(false);
	config.setAcceptPOST(false);
	config.setAcceptDELETE(false);
	while (token != end && token->type != SEMICOLON)
	{
		if (token->type != CONF_WORD)
			throw(std::runtime_error("Error in limit_except directive config"));
		else if (token->value == "GET" || token->value == "GET,")
			config.setAcceptGET(true);
		else if (token->value == "POST" || token->value == "POST,")
			config.setAcceptPOST(true);
		else if (token->value == "DELETE" || token->value == "DELETE,")
			config.setAcceptDELETE(true);
		else
			throw(std::runtime_error("Unknown method"));
		token++;
	}
}

void	Webserv::_parseMaxBodySize(std::list<t_conf_token>::iterator &token, std::list<t_conf_token>::iterator &end, Config &config)
{
	char		*end_num;
	long long	max_size;

	token++;
	if (token == end || token->type != CONF_WORD)
		throw(std::runtime_error("Error in client_max_body_size directive config"));
	max_size = strtoll(token->value.c_str(), &end_num, 10);
	if (max_size < 1)
		throw(std::runtime_error("Error in client_max_body_size directive config"));
	config.setMaxBody(max_size);
	if (errno == ERANGE)
	{
		errno = 0;
		throw(std::runtime_error("Error in client_max_body_size directive config"));
	}
	if (*end_num != '\0')
		throw(std::runtime_error("Error in client_max_body_size directive config"));
	token++;
	if (token == end || token->type != SEMICOLON)
		throw(std::runtime_error("Error in client_max_body_size directive config"));
}

void	Webserv::_parseRoot(std::list<t_conf_token>::iterator &token, std::list<t_conf_token>::iterator &end, Config &config)
{
	token++;
	if (token == end || token->type != CONF_WORD)
		throw(std::runtime_error("Error in root directive config"));
	if (token->value[0] != '/')
		throw(std::runtime_error("Error in root directive config"));
	while (*(token->value.rbegin()) == '/' && token->value.length() > 1)
		token->value.erase(token->value.length() - 1);
	config.setRoot(token->value);
	token++;
	if (token == end || token->type != SEMICOLON)
		throw(std::runtime_error("Error in root directive config"));
}

static bool	handledErrorCodes(int code)
{
	if (code == 400 || code == 403 || code == 404 || code == 405 || code == 408 || code == 409 || code == 411
		|| code == 411 || code == 413 || code == 414 || code == 500 || code == 502 || code == 503 || code == 505)
		return (true);
	return (false);
}

void	Webserv::_parseErrorPage(std::list<t_conf_token>::iterator &token, std::list<t_conf_token>::iterator &end, Config &config)
{
	char		*end_num;
	int			code;

	token++;
	if (token == end || token->type != CONF_WORD)
		throw(std::runtime_error("Error in error_page directive config"));
	code = strtol(token->value.c_str(), &end_num, 10);
	if (errno == ERANGE)
	{
		errno = 0;
		throw(std::runtime_error("Error in error_page directive config"));
	}
	if (*end_num != '\0' || !handledErrorCodes(code))
		throw(std::runtime_error("Error in error_page directive config"));
	token++;
	if (token == end || token->type != CONF_WORD)
		throw(std::runtime_error("Error in error_page directive config"));
	config.addErrorPage(code, token->value);
	token++;
	if (token == end || token->type != SEMICOLON)
		throw(std::runtime_error("Error in error_page directive config"));
}

void	Webserv::_parseAutoIndex(std::list<t_conf_token>::iterator &token, std::list<t_conf_token>::iterator &end, Config &config)
{
	token++;
	if (token == end || token->type != CONF_WORD)
		throw(std::runtime_error("Error in autoindex directive config"));
	if (token->value != "on" && token->value != "off")
		throw(std::runtime_error("Error in autoindex directive config"));
	if (token->value == "on")
		config.setAutoIndex(1);
	if (token->value == "off")
		config.setAutoIndex(0);
	token++;
	if (token == end || token->type != SEMICOLON)
		throw(std::runtime_error("Error in autoindex directive config"));
}

void	Webserv::_parseIndex(std::list<t_conf_token>::iterator &token, std::list<t_conf_token>::iterator &end, Config &config)
{
	token++;
	if (token == end || token->type != CONF_WORD)
		throw(std::runtime_error("Error in index directive config"));
	config.setIndex(token->value);
	token++;
	if (token == end || token->type != SEMICOLON)
		throw(std::runtime_error("Error in index directive config"));
}

void	Webserv::_parseRedir(std::list<t_conf_token>::iterator &token, std::list<t_conf_token>::iterator &end, Config &config)
{
	char		*end_num;
	int			status;

	token++;
	if (token == end || token->type != CONF_WORD)
		throw(std::runtime_error("Error in redirection directive config"));
	status = strtol(token->value.c_str(), &end_num, 10);
	if (errno == ERANGE)
	{
		errno = 0;
		throw(std::runtime_error("Error in redirection directive config"));
	}
	if (*end_num != '\0' || status < 300 || status > 399)
		throw(std::runtime_error("Error in redirection directive config"));
	token++;
	if (token == end || token->type != CONF_WORD)
		throw(std::runtime_error("Error in redirection directive config"));
	config.setRedirect(status, token->value);
	token++;
	if (token == end || token->type != SEMICOLON)
		throw(std::runtime_error("Error in redirection directive config"));
}

void	Webserv::_parseMaxCGIOutput(std::list<t_conf_token>::iterator &token, std::list<t_conf_token>::iterator &end, Config &config)
{
	char		*end_num;
	long long	max_size;

	token++;
	if (token == end || token->type != CONF_WORD)
		throw(std::runtime_error("Error in cgi_max_output_size directive config"));
	max_size = strtoll(token->value.c_str(), &end_num, 10);
	if (max_size < 1)
		throw(std::runtime_error("Error in cgi_max_output_size directive config"));
	config.setMaxCGIOutput(max_size);
	if (errno == ERANGE)
	{
		errno = 0;
		throw(std::runtime_error("Error in cgi_max_output_size directive config"));
	}
	if (*end_num != '\0')
		throw(std::runtime_error("Error in cgi_max_output_size directive config"));
	token++;
	if (token == end || token->type != SEMICOLON)
		throw(std::runtime_error("Error in cgi_max_output_size directive config"));
}

/*******************************************************************************
*						LOCATION BLOCK PARSING
*******************************************************************************/

void	Webserv::_parseUpload(std::list<t_conf_token>::iterator &token, std::list<t_conf_token>::iterator &end, Location &location)
{
	token++;
	if (token == end || token->type != CONF_WORD)
		throw(std::runtime_error("Error in upload_store directive config"));
	if (token->value[0] != '/')
		throw(std::runtime_error("Error in upload_store directive config"));
	while (*(token->value.rbegin()) == '/' && token->value.length() > 1)
		token->value.erase(token->value.length() - 1);
	location.setUploadStore(token->value);
	token++;
	if (token == end || token->type != SEMICOLON)
		throw(std::runtime_error("Error in upload_store directive config"));
}

void	Webserv::_parseLocation(std::list<t_conf_token>::iterator &token, std::list<t_conf_token>::iterator &end, Location &location)
{
	if (token == end || token->type != CONF_WORD)
		throw(std::runtime_error("Error in location config"));
	if (token->value[0] != '/')
		throw(std::runtime_error("Error in location config"));
	location.setPath(token->value);
	token++;
	if (token == end || token->type != OPEN_CURLY)
		throw(std::runtime_error("Error in location config"));
	token++;
	while (token != end && token->type != CLOSE_CURLY)
	{
		if (token->type == CONF_WORD && token->value == "root")
			_parseRoot(token, end, location);
		else if (token->type == CONF_WORD && token->value == "limit_except")
			_parseLimitExcept(token, end, location);
		else if (token->type == CONF_WORD && token->value == "autoindex")
			_parseAutoIndex(token, end, location);
		else if (token->type == CONF_WORD && token->value == "index")
			_parseIndex(token, end, location);
		else if (token->type == CONF_WORD && token->value == "client_max_body_size")
			_parseMaxBodySize(token, end, location);
		else if (token->type == CONF_WORD && token->value == "upload_store")
			_parseUpload(token, end, location);
		else if (token->type == CONF_WORD && token->value == "error_page")
			_parseErrorPage(token, end, location);
		else if (token->type == CONF_WORD && token->value == "return")
			_parseRedir(token, end, location);
		else if (token->type == CONF_WORD && token->value == "cgi_max_output_size")
			_parseMaxCGIOutput(token, end, location);
		else
			throw(std::runtime_error("Unknown directive"));
		token++;
	}
}

/*******************************************************************************
*						SERVER BLOCK PARSING
*******************************************************************************/

void	Webserv::_parseListen(std::list<t_conf_token>::iterator &token, std::list<t_conf_token>::iterator &end, Server &server)
{
	t_socket	socket;
	int			port_num;
	char		*end_num;

	token++;
	if (token == end || token->type != CONF_WORD)
		throw(std::runtime_error("Error in listen directive config"));
	if (token->value.find('/') != std::string::npos)
	{
		socket.ipAddr = token->value.substr(0, token->value.find('/'));
		socket.port = token->value.substr(token->value.find('/') + 1);
	}
	else
	{
		socket.ipAddr = "127.0.0.1";
		socket.port = token->value;
	}
	port_num = strtol(socket.port.c_str(), &end_num, 10);
	if (port_num < 1 || port_num > 65535 || *end_num != '\0')
		throw(std::runtime_error("Error in listen directive config"));
	server.addSocket(socket);
	token++;
	if (token == end || token->type != SEMICOLON)
		throw(std::runtime_error("Error in listen directive config"));
}

void	Webserv::_parseServerName(std::list<t_conf_token>::iterator &token, std::list<t_conf_token>::iterator &end, Server &server)
{
	token++;
	if (token == end || token->type != CONF_WORD)
		throw(std::runtime_error("Error in server_name directive config"));
	server.setName(token->value);
	token++;
	if (token == end || token->type != SEMICOLON)
		throw(std::runtime_error("Error in server_name directive config"));
}

void	Webserv::_parseServerBlock(std::list<t_conf_token>::iterator &token, std::list<t_conf_token>::iterator &end, Server &server)
{
	while (token != end && token->type != CLOSE_CURLY)
	{
		if (token->type == CONF_WORD && token->value == "location")
		{
			Location	location;
			token++;
			_parseLocation(token, end, location);
			if (token == end)
				throw(std::runtime_error("Syntax error in config file"));
			server.addLocation(location);
		}
		else if (token->type == CONF_WORD && token->value == "listen")
			_parseListen(token, end, server);
		else if (token->type == CONF_WORD && token->value == "server_name")
			_parseServerName(token, end, server);
		else if (token->type == CONF_WORD && token->value == "client_max_body_size")
			_parseMaxBodySize(token, end, server);
		else if (token->type == CONF_WORD && token->value == "root")
			_parseRoot(token, end, server);
		else if (token->type == CONF_WORD && token->value == "limit_except")
			_parseLimitExcept(token, end, server);
		else if (token->type == CONF_WORD && token->value == "error_page")
			_parseErrorPage(token, end, server);
		else if (token->type == CONF_WORD && token->value == "autoindex")
			_parseAutoIndex(token, end, server);
		else if (token->type == CONF_WORD && token->value == "index")
			_parseIndex(token, end, server);
		else if (token->type == CONF_WORD && token->value == "return")
			_parseRedir(token, end, server);
		else if (token->type == CONF_WORD && token->value == "cgi_max_output_size")
			_parseMaxCGIOutput(token, end, server);
		else
			throw(std::runtime_error("Unknown directive"));
		token++;
	}
}

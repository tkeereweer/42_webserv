#include "../include/Webserv.hpp"
#include <fstream>
#include <cctype>
#include <iostream>
#include <cstdlib>
#include <unistd.h>

std::string	Webserv::openFile(char const *filepath)
{
	std::ifstream	ifs(filepath);
	std::string		line;
	std::string		content;

	if (!ifs.is_open())
		throw(std::runtime_error("Unable to open file"));
	std::cout << "File opened" << std::endl;
	while (std::getline(ifs, line))
		content.append(line);
	// std::cout << content << std::endl;
	return (content);
}

std::list<t_conf_token>	Webserv::lexConfigFile(std::string content)
{
	std::list<t_conf_token>	out;
	t_conf_token			curr;

	for (std::string::iterator it = content.begin(); it != content.end(); it++)
	{
		if (!isspace(*it) && *it != '{' && *it != '}' && *it != ';')
		{
			curr.type = WORD;
			curr.value.push_back(*it);
		}
		else
		{
			if (curr.type == WORD && !curr.value.empty())
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

void	Webserv::printConfTokens(std::list<t_conf_token> lst)
{
	for (std::list<t_conf_token>::iterator it = lst.begin(); it != lst.end(); it++)
	{
		std::cout << it->value << std::endl;
	}
}

void	Webserv::parseListen(std::list<t_conf_token>::iterator &token, Server &server)
{
	t_socket	socket;

	token++;
	if (token->type != WORD)
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
	server.addSocket(socket);
	token++;
	if (token->type != SEMICOLON)
		throw(std::runtime_error("Error in listen directive config"));
}

void	Webserv::parseServerName(std::list<t_conf_token>::iterator &token, Server &server)
{
	token++;
	if (token->type != WORD)
		throw(std::runtime_error("Error in server_name directive config"));
	server.setName(token->value);
	token++;
	if (token->type != SEMICOLON)
		throw(std::runtime_error("Error in server_name directive config"));
}

void	Webserv::parseMaxBodySize(std::list<t_conf_token>::iterator &token, Server &server)
{
	char	*end;

	token++;
	if (token->type != WORD)
		throw(std::runtime_error("Error in client_max_body_size directive config"));
	server.setMaxBody(strtoull(token->value.c_str(), &end, 10));
	if (errno == ERANGE || *end != '\0')
		throw(std::runtime_error("Error in client_max_body_size directive config"));
	token++;
	if (token->type != SEMICOLON)
		throw(std::runtime_error("Error in client_max_body_size directive config"));
}

void	Webserv::parseServerRoot(std::list<t_conf_token>::iterator &token, Server &server)
{
	token++;
	if (token->type != WORD)
		throw(std::runtime_error("Error in root directive config"));
	if (token->value[0] != '/')
		throw(std::runtime_error("Error in root directive config"));
	while (*(token->value.rbegin()) == '/')
		token->value.erase(token->value.length() - 1);
	server.setServerRoot(token->value);
	token++;
	if (token->type != SEMICOLON)
		throw(std::runtime_error("Error in root directive config"));
}

void	Webserv::parseLocRoot(std::list<t_conf_token>::iterator &token, Location &location)
{	
	token++;
	if (token->type != WORD)
		throw(std::runtime_error("Error in root directive config"));
	if (token->value[0] != '/')
		throw(std::runtime_error("Error in root directive config"));
	while (*(token->value.rbegin()) == '/')
		token->value.erase(token->value.length() - 1);
	location.setLocRoot(token->value);
	token++;
	if (token->type != SEMICOLON)
		throw(std::runtime_error("Error in root directive config"));
}

void	Webserv::parseLimitExcept(std::list<t_conf_token>::iterator &token, Location &location)
{
	token++;
	if (token->type != WORD)
		throw(std::runtime_error("Error in limit_except directive config"));
	while (token->type != SEMICOLON)
	{
		if (token->type != WORD)
			throw(std::runtime_error("Error in limit_except directive config"));
		else if (token->value == "GET" || token->value == "GET,")
			location.setAcceptGET(true);
		else if (token->value == "POST" || token->value == "POST,")
			location.setAcceptPOST(true);
		else if (token->value == "DELETE" || token->value == "DELETE,")
			location.setAcceptDELETE(true);
		else
			throw(std::runtime_error("Unknown method"));
		token++;
	}
}

void	Webserv::parseErrorPage(std::list<t_conf_token>::iterator &token, Location &location)
{
	char		*end;
	int			code;

	token++;
	if (token->type != WORD)
		throw(std::runtime_error("Error in error_page directive config"));
	code = strtol(token->value.c_str(), &end, 10);
	if (errno == ERANGE || *end != '\0' || code < 400 || code > 499)
		throw(std::runtime_error("Error in error_page directive config"));
	token++;
	if (token->type != WORD)
		throw(std::runtime_error("Error in error_page directive config"));
	location.addErrorPage(code, token->value);
	token++;
	if (token->type != SEMICOLON)
		throw(std::runtime_error("Error in error_page directive config"));
}

void	Webserv::parseAutoIndex(std::list<t_conf_token>::iterator &token, Location &location)
{
	token++;
	if (token->type != WORD)
		throw(std::runtime_error("Error in autoindex directive config"));
	if (token->value != "on" && token->value != "off")
		throw(std::runtime_error("Error in autoindex directive config"));
	if (token->value == "on")
		location.setAutoIndex(true);
	if (token->value == "off")
		location.setAutoIndex(false);
	token++;
	if (token->type != SEMICOLON)
		throw(std::runtime_error("Error in autoindex directive config"));
}

void	Webserv::parseIndex(std::list<t_conf_token>::iterator &token, Location &location)
{
	token++;
	if (token->type != WORD)
		throw(std::runtime_error("Error in index directive config"));
	location.setIndex(token->value);
	token++;
	if (token->type != SEMICOLON)
		throw(std::runtime_error("Error in index directive config"));
}

void	Webserv::parseUpload(std::list<t_conf_token>::iterator &token, Location &location)
{
	token++;
	if (token->type != WORD)
		throw(std::runtime_error("Error in upload_store directive config"));
	location.setUploadStore(token->value);
	token++;
	if (token->type != SEMICOLON)
		throw(std::runtime_error("Error in upload_store directive config"));
}

void	Webserv::parseRedir(std::list<t_conf_token>::iterator &token, Location &location)
{
	char		*end;
	int			status;

	token++;
	if (token->type != WORD)
		throw(std::runtime_error("Error in redirection directive config"));
	status = strtol(token->value.c_str(), &end, 10);
	if (errno == ERANGE || *end != '\0' || status < 300 || status > 399)
		throw(std::runtime_error("Error in redirection directive config"));
	token++;
	if (token->type != WORD)
		throw(std::runtime_error("Error in redirection directive config"));
	location.setRedirect(status, token->value);
	token++;
	if (token->type != SEMICOLON)
		throw(std::runtime_error("Error in redirection directive config"));
}

void	Webserv::parseLocation(std::list<t_conf_token>::iterator &token, Location &location)
{
	if (token->type != WORD)
		throw(std::runtime_error("Error in location config"));
	if (token->value[0] != '/')
		throw(std::runtime_error("Error in location config"));
	location.setPath(token->value);
	token++;
	if (token->type != OPEN_CURLY)
		throw(std::runtime_error("Error in location config"));
	token++;
	while (token->type != CLOSE_CURLY)
	{
		if (token->type == WORD && token->value == "root")
			parseLocRoot(token, location);
		else if (token->type == WORD && token->value == "limit_except")
			parseLimitExcept(token, location);
		else if (token->type == WORD && token->value == "autoindex")
			parseAutoIndex(token, location);
		else if (token->type == WORD && token->value == "index")
			parseIndex(token, location);
		else if (token->type == WORD && token->value == "upload_store")
			parseUpload(token, location);
		else if (token->type == WORD && token->value == "error_page")
			parseErrorPage(token, location);
		else if (token->type == WORD && token->value == "return")
			parseRedir(token, location);
		else
			throw(std::runtime_error("Unknown directive"));
		token++;
	}
}

void	Webserv::parseServerBlock(std::list<t_conf_token>::iterator &token, Server &server)
{
	while (token->type != CLOSE_CURLY)
	{
		if (token->type == WORD && token->value == "location")
		{
			Location	location;
			token++;
			parseLocation(token, location);
			server.addLocation(location);
		}
		else if (token->type == WORD && token->value == "listen")
			parseListen(token, server);
		else if (token->type == WORD && token->value == "server_name")
			parseServerName(token, server);
		else if (token->type == WORD && token->value == "client_max_body_size")
			parseMaxBodySize(token, server);
		else if (token->type == WORD && token->value == "root")
			parseServerRoot(token, server);
		else
			throw(std::runtime_error("Unknown directive"));
		token++;
	}
}

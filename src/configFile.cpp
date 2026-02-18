#include "../include/Webserv.hpp"
#include <fstream>
#include <cctype>
#include <iostream>
#include <cstdlib>

std::string	openFile(char const *filepath)
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

bool	is_space(char const c)
{
	if (c != ' ' && c != '\t' && c != '\n' && c != '\v' && c != '\f' && c != '\r')
		return (false);
	return (true);
}

std::list<t_conf_token>	lexConfigFile(std::string content)
{
	std::list<t_conf_token>	out;
	t_conf_token			curr;

	for (std::string::iterator it = content.begin(); it != content.end(); it++)
	{
		if (!is_space(*it) && *it != '{' && *it != '}' && *it != ';')
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

void	printConfTokens(std::list<t_conf_token> lst)
{
	for (std::list<t_conf_token>::iterator it = lst.begin(); it != lst.end(); it++)
	{
		std::cout << it->value << std::endl;
	}
}

void	parseListen(std::list<t_conf_token>::iterator &token, Server &server)
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

void	parseServerName(std::list<t_conf_token>::iterator &token, Server &server)
{
	token++;
	if (token->type != WORD)
		throw(std::runtime_error("Error in server_name directive config"));
	server.setName(token->value);
	token++;
	if (token->type != SEMICOLON)
		throw(std::runtime_error("Error in server_name directive config"));
}

void	parseMaxBodySize(std::list<t_conf_token>::iterator &token, Server &server)
{
	char	*end;

	token++;
	if (token->type != WORD)
		throw(std::runtime_error("Error in client_max_body_size directive config"));
	server.setMaxBody(strtoull(token->value.c_str(), &end, 10));
	if (errno == ERANGE)
		throw(std::runtime_error("Error in client_max_body_size directive config"));
	token++;
	if (token->type != SEMICOLON)
		throw(std::runtime_error("Error in client_max_body_size directive config"));
}

void	parseServerRoot(std::list<t_conf_token>::iterator &token, Server &server)
{
	token++;
	if (token->type != WORD)
		throw(std::runtime_error("Error in root directive config"));
	// if (access(token->value.c_str(), R_OK) == -1)
	// 	throw(std::runtime_error("Error in root directive config"));
	server.setServerRoot(token->value);
	token++;
	if (token->type != SEMICOLON)
		throw(std::runtime_error("Error in root directive config"));
}

void	parseLocRoot(std::list<t_conf_token>::iterator &token, Location &location)
{	
	token++;
	if (token->type != WORD)
		throw(std::runtime_error("Error in root directive config"));
	// if (access(token->value.c_str(), R_OK) == -1)
	// 	throw(std::runtime_error("Error in root directive config"));
	location.setLocRoot(token->value);
	token++;
	if (token->type != SEMICOLON)
		throw(std::runtime_error("Error in root directive config"));
}

void	parseLimitExcept(std::list<t_conf_token>::iterator &token, Location &location)
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

// void	parseErrorPage(std::list<t_conf_token>::iterator &token, Location &location)
// {}

void	parseAutoIndex(std::list<t_conf_token>::iterator &token, Location &location)
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

void	parseIndex(std::list<t_conf_token>::iterator &token, Location &location)
{
	token++;
	if (token->type != WORD)
		throw(std::runtime_error("Error in index directive config"));
	// add valid path check?
	location.setIndex(token->value);
	token++;
	if (token->type != SEMICOLON)
		throw(std::runtime_error("Error in index directive config"));
}

void	parseUpload(std::list<t_conf_token>::iterator &token, Location &location)
{
	token++;
	if (token->type != WORD)
		throw(std::runtime_error("Error in upload_store directive config"));
	// add valid path check?
	location.setUploadStore(token->value);
	token++;
	if (token->type != SEMICOLON)
		throw(std::runtime_error("Error in upload_store directive config"));
}

// void	parseRedir(std::list<t_conf_token>::iterator &token, Location &location)
// {}

void	parseLocation(std::list<t_conf_token>::iterator &token, Location &location)
{
	if (token->type != WORD)
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
		{}
		else if (token->type == WORD && token->value == "return")
		{}
		else
			throw(std::runtime_error("Unknown directive"));
		token++;
	}
}

void	parseServerBlock(std::list<t_conf_token>::iterator &token, Server &server)
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

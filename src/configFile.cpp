#include "../include/Webserv.hpp"
#include <fstream>
#include <cctype>
#include <iostream>

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



#include "CGI.hpp"

t_cgi_token	whichType(std::string word)
{
	switch (word[0])
	{
		case ':':
			return (CGI_COLON);
		case ';':
			return (CGI_SEMI_COLON);
		case '/':
			return (CGI_SLASH);
	}
	return (CGI_WORD);
}

static t_cgiToken	token(std::string val, t_cgi_token type)
{
	t_cgiToken res;

	res.type = type;
	res.val = val;
	return (res);
}

void	CGI::_lexInput(std::string const &str)
{
	std::string::const_iterator	start = str.begin() + str.find_first_not_of("\n\r\t\b ");
	std::string	word = "";
	bool	inQuotes = false;

	for (std::string::const_iterator it = start; it != str.end(); it++)
	{
		if (*it == '"' && inQuotes)
		{
			inQuotes = false;
			word += *it;
			this->_CGItokenList.push_back(token(word, CGI_QUOTED));
			word.clear();
		}
		else if (*it == '"' && !inQuotes)
		{
			inQuotes = true;
			word += *it;
		}			
		else if (isspace(*it) && !inQuotes)
		{
			if (word != "")
			{
				this->_CGItokenList.push_back(token(word, CGI_WORD));
				word.clear();
			}
			if (*it == '\n')
			{
				this->_CGItokenList.push_back(token(std::string("\n"), CGI_LB));
				continue;
			}
			word += *it;
			this->_CGItokenList.push_back(token(word, CGI_SPACE));
			word.clear();
		}
		else if (!inQuotes && (*it == ':' || *it == ';' || *it == '/'))
		{
			if (word != "")
			{
				this->_CGItokenList.push_back(token(word, CGI_WORD));
				word.clear();
			}
			word += *it;
			this->_CGItokenList.push_back(token(word, whichType(word)));
			word.clear();
		}
		else
			word += *it;
		continue;
	}
	if (inQuotes)
		throw(std::runtime_error("unclosed quotes"));
	if (word != "")
		this->_CGItokenList.push_back(token(word, CGI_WORD));
}

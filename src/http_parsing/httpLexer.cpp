#include "Request.hpp"

bool	isCRLF(std::string::const_iterator	&it)
{
	return ((*it == '\r') && (*(it + 1) == '\n'));
}

static t_reqToken	token(std::string val, enum e_reqType type)
{
	t_reqToken res;

	res.type = type;
	res.val = val;
	return (res);
}

static enum e_reqType	whichType(std::string word)
{
	switch (word[0])
	{
		case ':':
			return (COLON);
		case ';':
			return (SEMI_COLON);
		case '/':
			return (SLASH);
		case '?':
			return (QMARK);
	}
	return (WORD);
}

void	Request::_lexInput(std::string const &str)
{
	std::string::const_iterator	start;
	if (str.find_first_not_of("\n\r\t\b ") == std::string::npos)
	{
		if (str.find_first_of("\n\r\t\b ") == std::string::npos)
			start = str.end();
		else
			start = str.begin();
	}
	else
		start = str.begin() + str.find_first_not_of("\n\r\t\b ");
	std::string	word = "";
	bool	inQuotes = false;

	for (std::string::const_iterator it = start; it != str.end(); it++)
	{
		if (*it == '"' && inQuotes)
		{
			inQuotes = false;
			word += *it;
			this->_tokenList.push_back(token(word, QUOTED));
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
				this->_tokenList.push_back(token(word, WORD));
				word.clear();
			}
			if (isCRLF(it))
			{
				if (this->_tokenList.back().type == CRLF)
				{
					this->_tokenList.push_back(token(std::string("\r\n"), CRLF));
					it++;
					it++;
					this->_tokenList.push_back(token(std::string(it, str.end()), WORD));
					break;
				}
				this->_tokenList.push_back(token(std::string("\r\n"), CRLF));
				it++;
				continue;
			}
			word += *it;
			this->_tokenList.push_back(token(word, SPACE));
			word.clear();
		}
		else if (!inQuotes && (*it == ':' || *it == ';' || *it == '/'|| *it == '?'))
		{
			if (word != "")
			{
				this->_tokenList.push_back(token(word, WORD));
				word.clear();
			}
			word += *it;
			this->_tokenList.push_back(token(word, whichType(word)));
			word.clear();
		}
		else
			word += *it;
		continue;
	}
	if (inQuotes)
		throw(std::runtime_error("unclosed quotes"));
	if (word != "")
		this->_tokenList.push_back(token(word, WORD));
}

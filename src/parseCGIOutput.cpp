#include "../include/CGI.hpp"

static t_cgi_token	whichType(std::string word)
{
	switch (word[0])
	{
		case ':':
			return (CGI_COLON);
	}
	return (CGI_WORD);
}

t_cgiToken	token(std::string val, t_cgi_token type)
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
				this->_CGItokenList.push_back(token(std::string("\r\n"), CGI_LB));
				it++;
				continue;
			}
			word += *it;
			this->_CGItokenList.push_back(token(word, CGI_SPACE));
			word.clear();
		}
		else if (!inQuotes && (*it == ':' || *it == ';' || *it == '/'|| *it == '?'))
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

void    CGI::_parseCGIOutput(std::list<t_cgiToken>::iterator &it)
{
    (void)it;
    std::cout << "we ain't s'ppose to be here, cowboy...\n";
}

//Returns -1 if request complete, 0 if expecting more headers, # of bytes to read until end of header.
//Don't call again if -1 or >0
//Consumes data such that data=="leftover after CGI_LB" after each call.
int	CGI::lexCGIOutput(std::string &data)
{
	if (data == "")
		throw(std::runtime_error("empty data field"));

	this->_lexInput(data);

	std::list<t_cgiToken>::reverse_iterator	rit = this->_CGItokenList.rbegin();
	while (rit->type != CGI_LB && rit != this->_CGItokenList.rend())
		rit++;
	if (rit == this->_CGItokenList.rend())
		return (0);
	//throw exception only if parsing interupted on bad grammar
	try
	{   
        std::list<t_cgiToken>::iterator it = this->_CGItokenList.begin();
		_parseCGIOutput(it);
	}
	catch(std::exception const &e)
	{
		throw(std::runtime_error(e.what()));
	}

	if (this->_outComplete)
		return (-1);
	data.clear(); //issue here where we have deleted token list and we then clear data, losing info

	//pop back in data last potentially unread token then pop_back()
	if (!this->_CGItokenList.empty() && (this->_CGItokenList.back().type == CGI_WORD || this->_CGItokenList.back().type == CGI_SPACE))
	{
		data.append(this->_CGItokenList.back().val.begin(), this->_CGItokenList.back().val.end());
		this->_CGItokenList.pop_back();
	}

	if (!(this->_outHeadersValid && this->_outLineValid))
		return (0);
	else
		return (this->_contentLength - this->_bytesRead);
}

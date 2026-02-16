#include "../../include/Request.hpp"

bool	isCRLF(std::string::const_iterator	&it)
{
	return ((*it == '\r') && (*(it + 1) == '\n'));
}

std::vector<std::string>	&lexInput(std::string const &str)
{
	std::string::const_iterator	start = str.begin() + str.find_first_not_of("\n\r\t\b ");
	std::vector<std::string>	res;
	std::string	word = "";
	bool	inQuotes = false;

	for (std::string::const_iterator it = start; it != str.end(); it++)
	{
		if (*it == '"')
		{
			if (inQuotes)
			{
				inQuotes = false;
				word += *it;
				res.push_back(word);
				word.clear();
			}
			else
				inQuotes = true;		
		}
		else if (isspace(*it) && !inQuotes)
		{
			res.push_back(word);
			word.clear();
			if (isCRLF(it))
				res.push_back(std::string(it, it + 1));
		}
		else if (*it == ':' && !inQuotes)
		{
			res.push_back(word);
			word.clear();
			res.push_back(std::string(":"));
		}
		else
			word += *it;
		continue;
	}

	if (inQuotes)
		throw(std::runtime_error("unclosed quotes"));
}

Request::Request(std::string &str)
{
	//lex the string into vector of strings
	std::vector<std::string>	tokenList = lexInput(str);


	try
	{
		_parse(str);
	}
	catch(const std::exception& e)
	{
		std::cerr << e.what() << '\n';
	}
}

void	Request::_parse(std::string &input)
{
	std::string			token;
	std::stringstream	stream(input);

	//parse request line
	


}

Request::~Request(void){}


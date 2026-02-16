#ifndef REQUEST_HPP
# define REQUEST_HPP

#include <string>
#include <sstream>
#include <iostream>
#include <vector>
#include <algorithm>
#include <cctype>

//headers are divided like in RFC 1945
//only POST has a body
class Request
{
	private:

		//if we can have multiple times the same header, better to have a map of headers ?

		//request line
		std::string	_method;
		std::string	_URI;
		std::string	_HTTPVersion;
		
		//general headers
		std::string	_date;

		//request headers, info about the client

		//entity headers, info about entity-body
		std::string	_contentEncoding;
		std::string	_contentLength;
		std::string	_contentType;

		//cookies
		//key=value; key=value
		//session-ID cookie is a value used to mark login and therefore access to certain pages
		std::string	_cookies;

		std::string	_entityBody;

		Request(void);
		void	_parse(std::string &input);

	public:
		Request(std::string &str);
		Request(Request const &src);
		~Request(void);
		Request	&operator=(Request const &rhs);


};


#endif
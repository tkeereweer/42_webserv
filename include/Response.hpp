#ifndef RESPONSE_HPP
# define RESPONSE_HPP

#include <string>


class Response
{
	private:
		//start-line
		std::string _protocol;
		short		_errorCode;
		std::string	_reasonPhrase;

		//entity headers, info about entity-body
		std::string	_contentEncoding;
		std::string	_contentLength;
		std::string	_contentType;

		//cookies
		//there can be multiple values
		//key=value; Expires=Thu, <date> or Max-Age=<num in s>
		std::string	_setCookie;
		
		std::string	_entityBody;
};

#endif
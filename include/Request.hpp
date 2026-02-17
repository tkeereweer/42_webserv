#ifndef REQUEST_HPP
# define REQUEST_HPP

#include <string>
#include <sstream>
#include <iostream>
#include <list>
#include <algorithm>
#include <cctype>
#include <fstream>
#include <dirent.h>
#include <cstring>
#include <unistd.h>
#include <limits>
#include <stdlib.h>

enum e_reqType
{
	WORD,
	QUOTED,
	SPACE,
	CRLF,
	COLON,
	SLASH,
	SEMI_COLON,
	QMARK
};

typedef struct s_reqToken
{
	enum e_reqType type;
	std::string	val;
}t_reqToken;


//headers are divided like in RFC 1945
//only POST has a body
class Request
{
	private:
		//request line
		std::string	_method;
		std::string	_URI;
		std::string	_HTTPVersion;
		
		//general headers
		std::string	_date;

		//request headers, info about the client

		//entity headers, info about entity-body
		std::string	_contentEncoding;
		int         _contentLength;
		std::string	_contentType;

		//cookies
		//key=value; key=value
		//session-ID cookie is a value used to mark login and therefore access to certain pages
		std::string	_cookies;

		std::string	_bodyFilename;

		std::list<t_reqToken>	_tokenList;
		//flag to indicate we got to the end of request headers
		bool					_reqComplete;

		void	_createTempFile(void);
		void	_lexInput(std::string const &str);
		int		_requestEval(std::string &data);
		//takes leftover from parsing, puts it as is in Body and returns body content_length - length
		int		_leftToRead(void);

		//PARSING METHODS
		//parses until 2xCRLF. Leftover is still in tokenList
		void	    _parse(void);
		void	    _parseSimpleRequest(void);
		void        _parseURI(std::list<t_reqToken>::iterator &it);
		std::string _parseAbsPath(std::list<t_reqToken>::iterator &it);
		std::string	_parsePath(std::list<t_reqToken>::iterator &it);
		std::string	_parseParams(std::list<t_reqToken>::iterator &it);
		std::string	_parseQuery(std::list<t_reqToken>::iterator &it);
		std::string	_parseFSegment(std::list<t_reqToken>::iterator &it);
		std::string	_parseSegment(std::list<t_reqToken>::iterator &it);
		std::string	_parseParam(std::list<t_reqToken>::iterator &it);


	public:
		Request(void);
		Request(Request const &src);
		~Request(void);
		Request	&operator=(Request const &rhs);

		int     	lexRawData(std::string &data);
		int			fillBody(std::string &data);
		void		parse(std::list<std::string> &input);


		static bool	isCRLF(std::string::const_iterator	&it);
		


};



#endif
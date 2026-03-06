#ifndef REQUEST_HPP
# define REQUEST_HPP

# include "libraryHeader.hpp"

typedef enum	e_method
{
	EMPTY,
	GET,
	POST,
	DELETE
}	t_method;

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
}	t_reqToken;


//headers are divided like in RFC 1945
//only POST has a body
class Request
{
	private:
		//request line
		t_method	_method;
		std::string	_URI;
		std::string	_HTTPVersion;
		std::string	_contentEncoding;
		long long	_contentLength;
		std::string	_contentType;
		//cookies
		//key=value; key=value
		//session-ID cookie is a value used to mark login and therefore access to certain pages
		std::string	_cookies;
		std::string	_bodyFilename;
		std::list<t_reqToken>	_tokenList;
		//flag to indicate we got to the end of request headers
		bool    _reqComplete;
		bool    _reqLineValid;      
		bool    _reqHeadersValid;         
		//body bytes already read such that after body consumed, contentLength - bytesRead == 0
		long long	    _bytesRead;
		struct timeval	_recvTimestamp;
		std::string     _queryParam;


		void	_lexInput(std::string const &str);
		//takes leftover from parsing, puts it as is in Body and returns body content_length - length
		void	_readLeftovers(std::list<t_reqToken>::iterator &it);
		void	_createTempFile(void);
		void	_createNextAvailableFile(struct dirent *name, DIR *tmp);

		/*
		~~~~~ PARSING FUNCTIONS ~~~~~
		*/

		//parses until 2xCRLF. Leftover is still in tokenList
		void	    _parse(void);
		void	    _parseSimpleRequest(void);
		void		_parseFullRequest(std::list<t_reqToken>::iterator &it);
		void        _parseURI(std::list<t_reqToken>::iterator &it);
		std::string _parseAbsPath(std::list<t_reqToken>::iterator &it);
		std::string	_parsePath(std::list<t_reqToken>::iterator &it);
		std::string	_parseParams(std::list<t_reqToken>::iterator &it);
		std::string	_parseQuery(std::list<t_reqToken>::iterator &it);
		std::string	_parseFSegment(std::list<t_reqToken>::iterator &it);
		std::string	_parseSegment(std::list<t_reqToken>::iterator &it);
		std::string	_parseParam(std::list<t_reqToken>::iterator &it);
		void		_parseRequestLine(std::list<t_reqToken>::iterator &it);
		void		_parseHTTPVersion(std::list<t_reqToken>::iterator &it);
		bool		_parseContentEncoding(std::list<t_reqToken>::iterator &it);
		std::string	_parseContentCoding(std::list<t_reqToken>::iterator &it);
		bool		_parseContentLength(std::list<t_reqToken>::iterator &it);
		bool		_parseContentType(std::list<t_reqToken>::iterator &it);
		std::string	_parseMediaType(std::list<t_reqToken>::iterator &it);
		bool		_parseCookies(std::list<t_reqToken>::iterator &it);


	public:
		Request(void);
		Request(Request const &src);
		~Request(void);

		//overloads
		Request	        &operator=(Request const &rhs);

		int     	lexRawData(std::string &data);
		
		//getters
		t_method const		&getMethod(void) const;
		std::string const	&getURI(void) const;
		std::string const	&getHTTPVersion(void) const;
		std::string const	&getContentEncoding(void) const;
		long long const		&getContentLength(void) const;
		std::string const	&getContentType(void) const;
		std::string const	&getCookies(void) const;
		std::string const	&getBodyFilename(void) const;
		struct timeval		getRecvTimestamp(void) const;
		bool				getHeaderFlag(void) const;
		bool				getReqFlag(void) const;
		std::string			&getQueryParam(void);
		long long			getBytesRead(void) const;

		//setters
		void				setRecvTimestamp(struct timeval time);
		void                setURI(std::string path);
		void                setQueryParam(std::string params);
		void				setReqFlag(bool state);

		void				addBytesRead(long long size);
};

std::ostream    &operator<<(std::ostream &stream, Request const &rhs);

bool	isToken(std::string &str);
bool	isValidForHeaders(std::string &str);


#endif
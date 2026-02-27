#ifndef RESPONSE_HPP
# define RESPONSE_HPP

#include "libraryHeader.hpp"
#include "CGI.hpp"

class Server;
class Client;

class Response
{
	private:
		//start-line
		std::string _protocol;
		short		_returnCode;
		std::string	_reasonPhrase;
		std::string _allow; //only for error405

		//entity headers, info about entity-body
		std::string	_contentEncoding;
		std::string _contentLength;
		size_t		_toRead; //contentLength converted once to size_t for easier send operations
		std::string	_contentType;
		std::string _location; //for redirections
		//cookies
		//there can be multiple values
		//key=value; Expires=Thu, <date> or Max-Age=<num in s>
		std::string	_setCookie;
		
		std::string	_entityBody;
		std::string _bodyFilepath; //path of file to write in _entityBody

		//where we build the response and from which we will write to client
		std::string 	_rawResponse;
		bool    		_responseComplete;
		struct timeval	_sendTimestamp;

		void    _buildRawResponse(void);
		void    _writeFileToResponse(std::string filepath);

	
	public:
		Response(void);
		Response(Response const &src);
		~Response(void);
		Response    &operator=(Response const &rhs);

		void    buildErrorResponse(short code);
		void    build405Response(bool getAllowed, bool postAllowed, bool deleteAllowed);
		void    buildRouteResponse(std::string localPath);
		void    buildRedirResponse(std::string redirPath);
		void    buildGetCGIResponse(Client &client, int epollFD, Server &server, std::string scriptPath);
		void	buildPostCgiResponse(void);

		//getter
		std::string 	&getRawResponse(void);
		std::string 	getContentLength(void) const;
		struct timeval	&getSendTimestamp(void);
		bool			getRespFlag(void) const;
		size_t			getToRead(void) const;

		//setter
		void	setSendTimestamp(struct timeval timestamp);
};

#endif
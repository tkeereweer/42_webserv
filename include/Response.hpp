#ifndef RESPONSE_HPP
# define RESPONSE_HPP

#include <string>
#include <iostream>
#include <list>


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
		std::string	_contentType;
        std::string _location; //for redirections

		//cookies
		//there can be multiple values
		//key=value; Expires=Thu, <date> or Max-Age=<num in s>
		std::string	_setCookie;
		
		std::string	_entityBody;

        //where we build the response and from which we will write to client
        std::string _rawResponse;

        bool    _responseComplete;

        void    _buildRawResponse(void);

    
    public:
        Response(void);
        Response(Response const &src);
        ~Response(void);
        Response    &operator=(Response const &rhs);

        void    buildErrorResponse(short code);
        void    build405Response(bool getAllowed, bool postAllowed, bool deleteAllowed);
        void    buildRouteResponse(std::string localPath);
        void    buildRedirResponse(std::string redirPath);
};

#endif
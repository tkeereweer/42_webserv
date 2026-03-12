#include "Webserv.hpp"
#include "Server.hpp"

void	Response::buildRawResponse(void)
{
	std::stringstream returnCodeStr;

	this->_rawResponse += this->_protocol + " ";
	returnCodeStr << this->_returnCode;
	this->_rawResponse += returnCodeStr.str() + " " + this->_reasonPhrase + "\r\n";
	if (!this->_bodyFilepath.empty())
		_writeFileToResponse(this->_bodyFilepath);
	if (!this->_location.empty())
	{
		this->_rawResponse += "Location: ";
		this->_rawResponse += this->_location;
		this->_rawResponse += "\r\n";
	}
	if (this->_contentEncoding != "")
	{
		this->_rawResponse += "Content-Encoding: ";
		this->_rawResponse += this->_contentEncoding;
		this->_rawResponse += "\r\n";
	}
	if (this->_contentLength != "")
	{
		this->_rawResponse += "Content-Length: ";
		this->_rawResponse += this->_contentLength;
		this->_rawResponse += "\r\n";
	}
	if (this->_contentType != "")
	{
		this->_rawResponse += "Content-Type: ";
		this->_rawResponse += this->_contentType;
		this->_rawResponse += "\r\n";
	}
	if (this->_setCookie != "")
	{
		this->_rawResponse += "Set-Cookie: ";
		this->_rawResponse += this->_setCookie;
		this->_rawResponse += "\r\n";
	}
	this->_rawResponse += "\r\n";
	this->_toRead += this->_rawResponse.size();
	if (this->_entityBody != "")
		this->_rawResponse += this->_entityBody;
	this->_responseComplete = true;
	return ;
}


void    Response::_writeFileToResponse(std::string filepath)
{
	size_t			readSize = 1024 * 1024; //1MB buffer
	std::ifstream	file(filepath.c_str(), std::ios::binary); //allows opening of file with binary data like jpegs
	if (!file.is_open())
	{
		std::cout << "file: " << filepath << " failed to open" <<std::endl;
		throw (std::runtime_error("500"));
	}
	//most memory efficient approach for large files
	//should we do "write" chunking of size == max_readable_chunk_per_TCP_packet ? THen use flags like for reading ?
	std::vector<char> buffer(readSize);
	long long	size = 0;
	file.read(&buffer[0], readSize);
	while (file.gcount() != 0)
	{
		size += file.gcount(); //number of bytes read
		this->_entityBody.insert(this->_entityBody.end(), buffer.begin(), buffer.end());
		buffer.clear();
		file.read(&buffer[0], readSize);
	}
	std::stringstream	sstr;
	sstr << size;
	this->_contentLength += sstr.str();
	this->_toRead = size;
	//set to empty for now as it is the server's responsibility to encode or not.
	//content length would refer to the encoded length.
	this->_contentEncoding = "";
	//for now, only text/html or text/css handled with this method
	std::string extension(&filepath[filepath.find_last_of(".") + 1]);
	this->_contentType = "text/";
	this->_contentType += extension;
	this->_contentType += "; charset=utf-8";
	return ;
}

//read the right ressource, set content type, content length and encoding and write in rawPath
//if CGI handling or cookie setup, do here
void    Response::buildRouteResponse(std::string localPath, Server *server, Location *loc)
{
	if (access(localPath.c_str(), R_OK) == -1)
		return (buildErrorResponse(404, server, loc));
	_writeFileToResponse(localPath);
	this->_returnCode = 200;
	this->_protocol = "HTTP/1.0";
	this->_reasonPhrase = "OK";
	return (buildRawResponse());
}
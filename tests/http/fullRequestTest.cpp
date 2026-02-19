#include "../../include/Request.hpp"

int main(int argc, char **argv)
{
	if (argc != 2)
		return (1);
	
	std::ifstream file(argv[1]);

	if (!file.is_open())
		return (1);
	std::ostringstream  sstr;
	sstr << file.rdbuf();
	std::string data(sstr.str());
	Request test;

	int res = -2;

	try
	{
		res = test.lexRawData(data);
	}
	catch(const std::exception& e)
	{
		std::cerr << e.what() << '\n';
		return (1);
	}
	std::cout << "Parsing success, return code:  " << res << std::endl;  

	std::cout << "method: " << test.getMethod() << std::endl;
	std::cout << "URI: " << test.getURI() << std::endl;
	std::cout << "HTTP-Version: " << test.getHTTPVersion() << std::endl;
	std::cout << "Content-Encoding: " << test.getContentEncoding() << std::endl;
	std::cout << "Content-Length: " << test.getContentLength() << std::endl;
	std::cout << "Content-Type: " << test.getContentType() << std::endl;
	std::cout << "Cookies: " << test.getCookies() << std::endl;
	std::cout << "Body filename: " << test.getBodyFilename() << std::endl;
    std::ifstream toRead(test.getBodyFilename().c_str());
    std::cout << "body content: " << std::endl;
    std::cout << toRead.rdbuf() << std::endl;
	return (0);
}
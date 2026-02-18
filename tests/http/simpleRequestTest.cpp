#include "../../include/Request.hpp"

int main(void)
{
	std::string request;

	if (!std::getline(std::cin, request))
		return (1);
	request += "\r\n";

	if (request.empty())
	{
		std::cerr << "Error: empty request" << std::endl;
		return (1);
	}

	int res = -2;
	Request test;
	try
	{
		res = test.lexRawData(request);
	}
	catch (const std::exception& e)
	{
		std::cerr << e.what() << '\n';
		return (1);
	}

	std::cout << "Parsing return code:  " << res << std::endl;

	return (0);
}
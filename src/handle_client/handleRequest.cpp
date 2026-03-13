#include "Webserv.hpp"

void	Webserv::_handleRequest(int clientFd)
{
	Client&		    client = _clientMap[clientFd].client;
	char		    buffer[4056];
	std::time_t	now;

	ssize_t bytesRead = recv(clientFd, buffer, sizeof(buffer) - 1, 0);
	//logic for timeout handling
	time(&now);
	client.getRequest().setRecvTimestamp(now);
	int	lexReturn = -2;

 	if (bytesRead < 1)
		return (_closeClient(clientFd));

	buffer[bytesRead] = '\0';
	// std::cout << "Recv from client (" << clientFd << ") : " << std::string(buffer);
	client.appendReadBuffer(std::string(buffer, bytesRead));//data string
	if (!client.getRequest().getHeaderFlag())
        lexReturn = _lexInput(client, clientFd);
	else
	{
		int tempFD = open(client.getRequest().getBodyFilename().c_str(), O_WRONLY | O_APPEND);
		if (write(tempFD, client.getReadBuffer().c_str(), client.getReadBuffer().size()) < 1)
		{
			lexReturn = -1;
			client.getRequest().setReqFlag(true);
			client.getResponse().buildErrorResponse(500, this->_clientMap[clientFd].server, NULL);
		}
		else
		{
			client.getRequest().addBytesRead(client.getReadBuffer().size());
			client.getReadBuffer().clear();
			if (client.getRequest().getContentLength() - client.getRequest().getBytesRead() <= 0)
			{
				lexReturn = -1;
				client.getRequest().setReqFlag(true);
				if (client.getRequest().getContentLength() - client.getRequest().getBytesRead() < 0) //content-length < body size
					client.getResponse().buildErrorResponse(400, this->_clientMap[clientFd].server, NULL);
			}
		}
		close(tempFD);

	}
	if (lexReturn == -1)
	{
		this->_clientMap[clientFd].server->dispatchRequest(client, this->_epollFd); //CGI added to epoll ctl in this function

		if (this->_clientMap[clientFd].client.getCgiResponseState() == 0)
            _modifyEpoll(EPOLLOUT, EPOLL_CTL_MOD, clientFd);
		else //remove clientFD from epoll while we are reading the content from the CGI
		{
			struct epoll_event	event;
			epoll_ctl(_epollFd, EPOLL_CTL_DEL, clientFd, &event);
		}
	}
}

int Webserv::_lexInput(Client &client, int clientFd)
{
    int ret;
    try
    {
        ret = client.getRequest().lexRawData(client.getReadBuffer());
    }
    catch(const Request::Error405 &e)
    {
        client.getResponse().build405Response(true, true, true, this->_clientMap[clientFd].server, NULL);
        ret = -1; //to get into write response logic
    }
    catch(const Request::ErrorNum &e)
    {
        client.getResponse().buildErrorResponse(e.getCode(), this->_clientMap[clientFd].server, NULL);
        ret = -1; //to get into write response logic
    }
    catch(const std::exception& e)
    {
        std::cerr << e.what() << '\n';
        client.getResponse().buildErrorResponse(500, this->_clientMap[clientFd].server, NULL);
        ret = -1; //to get into write response logic
    }
    return (ret);
}
#include "../../include/Request.hpp"

void	Request::_createNextAvailableFile(struct dirent *name, DIR *tmp)
{
	std::string lastFile(name->d_name);
	int num = atoi(std::string(&lastFile[lastFile.find_last_of("_") + 1]).c_str()); //first num found
	int next_num = 0;
	std::ostringstream numWrite;
	std::string	nameToFind = "wbsrv_rqst_";

	//exit loop if no file found or intmax number of temp files
	while (name && next_num < std::numeric_limits<int>::max())
	{
		nameToFind = "wbsrv_rqst_";
		numWrite << next_num;
		nameToFind += numWrite.str();
		//finds file with nex number
		while (name && strncmp(name->d_name, nameToFind.c_str(), strlen(nameToFind.c_str())))
			name = readdir(tmp);
		next_num++;
		numWrite.str("");
	}
	if (errno != 0)
		throw(std::runtime_error(strerror(errno)));
	if (num == std::numeric_limits<int>::max())
		throw(std::runtime_error("out of room for temp files"));
	
	//create available file found
	std::string	filename("/tmp/");
	filename += nameToFind;
	this->_bodyFilename = filename;
	std::ofstream file(filename.c_str());
	if (!file)
			throw(std::runtime_error("can't open file"));
	return ;	
}

void    Request::_createTempFile(void)
{
	if (this->_bodyFilename != "")
		return ;
	
	DIR *tmp = opendir("/tmp");
	if (tmp == NULL)
		throw(std::runtime_error(strerror(errno)));

	struct dirent *name = readdir(tmp);
	if (!name && errno != 0)
		throw(std::runtime_error(strerror(errno)));
	while (name && strncmp(name->d_name, "wbsrv_rqst_", strlen("wbsrv_rqst_")))
		name = readdir(tmp);
	if (errno != 0)
		throw(std::runtime_error(strerror(errno)));
	//no temp file, create from 0
	if (!name)
	{
		std::ofstream file("/tmp/wbsrv_rqst_0");
		if (!file)
			throw(std::runtime_error("can't open file"));
		return ;
	}

	//handle next available file logic
	return (_createNextAvailableFile(name, tmp));
}

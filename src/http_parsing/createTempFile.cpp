#include "../../include/Request.hpp"

void    Request::_createTempFile(void)
{
    if (this->_bodyFilename != "")
        return ;
    
    DIR *tmp = opendir("/tmp");
    if (tmp = NULL)
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
        std::ifstream file;
        file.open("wbsrv_rqst_0");
        this->_bodyFilename = "/tmp/wbsrv_rqst_0";
        file.close();
        return ;
    }

    std::string lastFile(name->d_name);
    int num = atoi(std::string(&lastFile[lastFile.find_last_of("_") + 1]).c_str()); //first num found
    int next_num = 0;
    while (num < std::numeric_limits<int>::max())
    {
        while (name && strncmp(name->d_name, "wbsrv_rqst_", strlen("wbsrv_rqst_")))
            name = readdir(tmp);
        if (!name)
            break;
        lastFile = name->d_name;
        next_num = atoi(std::string(&lastFile[lastFile.find_last_of("_") + 1]).c_str());
        if (next_num != num + 1)
            break;
    }
    if (errno != 0)
        throw(std::runtime_error(strerror(errno)));
    if (num == std::numeric_limits<int>::max())
        throw(std::runtime_error("out of room for temp files"));
    
    std::string filename("/tmp/wbsrv_rqst_");
    std::ostringstream number;
    number << num  + 1;
    filename += number.str();
    this->_bodyFilename = filename;
    std::ifstream file;
    file.open(filename);
    file.close();
    return ;
}

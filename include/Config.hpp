#ifndef CONFIG_HPP
# define CONFIG_HPP

# include "libraryHeader.hpp"

class	Config
{
	protected:
		bool						_acceptGET;
		bool						_acceptPOST;
		bool						_acceptDELETE;
		std::string					_root;
		int							_autoIndex;
		std::string					_index;
		long long					_maxBodySizeClientReq;
		std::map<int, std::string>	_errorPages;
		std::pair<int, std::string>	_redirect;
		long long					_cgiMaxOutputSize;

	public:
		Config(void);
		Config(Config const &src);
		Config	&operator=(Config const &rhs);
		virtual ~Config(void);

		bool						getAcceptGET(void) const;
		bool						getAcceptPOST(void) const;
		bool						getAcceptDELETE(void) const;
		std::string					getRoot(void) const;
		int							getAutoIndex(void) const;
		std::string					getIndex(void) const;
		long long					getMaxBody(void) const;
		std::map<int, std::string>	&getErrorPages(void);
		std::pair<int, std::string>	getRedir(void) const;
		long long					getMaxCGIOutput(void) const;

		void	setAcceptGET(bool accept);
		void	setAcceptPOST(bool accept);
		void	setAcceptDELETE(bool accept);
		void	setRoot(std::string root);
		void	setAutoIndex(int on);
		void	setIndex(std::string file);
		void	setMaxBody(long long maxBody);
		void	addErrorPage(int code, std::string page);
		void	setRedirect(int status, std::string path);
		void	setMaxCGIOutput(long long maxCGIOutput);
};

#endif
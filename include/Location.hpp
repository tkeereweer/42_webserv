#ifndef LOCATION_HPP
# define LOCATION_HPP

# include <string>
# include <map>
# include <utility>

typedef enum	e_method
{
	GET,
	POST,
	DELETE
}	t_method;

class	Location
{
	private:
		std::string					_path;
		bool						_acceptGET;
		bool						_acceptPOST;
		bool						_acceptDELETE;
		std::string					_root;
		bool						_autoIndex;
		std::string					_index;
		std::string					_uploadStore;
		std::map<int, std::string>	_errorPages;
		std::pair<int, std::string>	_redirect;

	public:
		Location(void);
		Location(Location const &src);
		Location	&operator=(Location const &rhs);
		~Location(void);

		std::string					getPath(void) const;
		bool						getAcceptGET(void) const;
		bool						getAcceptPOST(void) const;
		bool						getAcceptDELETE(void) const;
		std::string					getLocRoot(void) const;
		bool						getAutoIndex(void) const;
		std::string					getIndex(void) const;
		std::string					getUploadStore(void) const;
		std::map<int, std::string>	&getErrorPages(void);
		std::pair<int, std::string>	getRedir(void) const;

		void	setPath(std::string path);
		void	setAcceptGET(bool accept);
		void	setAcceptPOST(bool accept);
		void	setAcceptDELETE(bool accept);
		void	setLocRoot(std::string root);
		void	setAutoIndex(bool on);
		void	setIndex(std::string file);
		void	setUploadStore(std::string dir);
		void	addErrorPage(int code, std::string page);
		void	setRedirect(int status, std::string path);
};

#endif
#ifndef LOCATION_HPP
# define LOCATION_HPP

# include <string>

typedef enum	e_method
{
	GET,
	POST,
	DELETE
}	t_method;

class	Location
{
	private:
		std::string	_path;
		bool		_acceptGET;
		bool		_acceptPOST;
		bool		_acceptDELETE;
		std::string	_root;
		bool		_autoIndex;
		std::string	_index;
		std::string	_uploadStore;
		// error pages, redirections, others

	public:
		Location(void);
		Location(Location const &src);
		Location	&operator=(Location const &rhs);
		~Location(void);

		std::string	getPath(void) const;
		bool		getAcceptGET(void) const;
		bool		getAcceptPOST(void) const;
		bool		getAcceptDELETE(void) const;
		std::string	getLocRoot(void) const;
		bool		getAutoIndex(void) const;
		std::string	getIndex(void) const;
		std::string	getUploadStore(void) const;

		void	setPath(std::string path);
		void	setAcceptGET(bool accept);
		void	setAcceptPOST(bool accept);
		void	setAcceptDELETE(bool accept);
		void	setLocRoot(std::string root);
		void	setAutoIndex(bool on);
		void	setIndex(std::string file);
		void	setUploadStore(std::string dir);
};

#endif
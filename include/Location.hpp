#ifndef LOCATION_HPP
# define LOCATION_HPP

# include "Config.hpp"
# include "libraryHeader.hpp"

class	Location: public Config
{
	private:
		std::string					_path;
		std::string					_uploadStore;

	public:
		Location(void);
		Location(Location const &src);
		Location	&operator=(Location const &rhs);
		virtual ~Location(void);

		std::string					getPath(void) const;
		std::string					getUploadStore(void) const;

		void	                    setPath(std::string path);
		void	                    setUploadStore(std::string dir);
};

#endif
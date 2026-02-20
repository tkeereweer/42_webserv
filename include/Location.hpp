#ifndef LOCATION_HPP
# define LOCATION_HPP

# include <string>
# include <map>
# include <utility>

# include "Config.hpp"

class	Location: public Config
{
	private:
		std::string					_path;
		std::string					_uploadStore;

	public:
		Location(void);
		Location(Location const &src);
		Location	&operator=(Location const &rhs);
		~Location(void);

		std::string					getPath(void) const;
		std::string					getUploadStore(void) const;

		void	setPath(std::string path);
		void	setUploadStore(std::string dir);
};

#endif
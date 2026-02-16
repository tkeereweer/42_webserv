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
		t_method	_acceptedMethods[3];
		// the rest of the attributes

	public:
		Location(void);
		Location(Location const &src);
		Location	&operator=(Location const &rhs);
		~Location(void);
};

#endif
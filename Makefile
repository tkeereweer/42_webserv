SRC = Client.cpp \
	Location.cpp \
	Server.cpp \
	Webserv.cpp \
	configFile.cpp \
	main.cpp

NAME = webserv

CC = g++
FLAGS = -Wall -Werror -Wextra -std=c++98 -g
OBJ_DIR = objects
OBJS = $(addprefix $(OBJ_DIR)/, $(notdir $(SRC:.cpp=.o)))

all: $(NAME)

$(NAME): $(LIBFT) $(MLX) $(OBJS)
	$(CC) $(FLAGS) $(OBJS) $(LINKS_MAC) -o $(NAME)

$(OBJ_DIR)/%.o: %.cpp | $(OBJ_DIR) #for main.c
	$(CC) $(FLAGS) -Iinclude -c $< -o $@

$(OBJ_DIR)/%.o: src/%.cpp | $(OBJ_DIR)
	$(CC) $(FLAGS) -Iinclude -c $< -o $@

$(OBJ_DIR):
	mkdir -p $(OBJ_DIR)

clean:
	rm -rf $(OBJ_DIR)

fclean: clean
	rm -rf $(NAME)

re: fclean all

.PHONY: all clean fclean re
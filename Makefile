SRC = Client.cpp \
	Location.cpp \
	Server.cpp \
	Webserv.cpp \
	Config.cpp \
	configFile.cpp \
	http_parsing/createTempFile.cpp \
	http_parsing/httpLexer.cpp \
	http_parsing/lexerUtils.cpp \
	http_parsing/parsingMethods.cpp \
	http_parsing/Request.cpp \
	Response.cpp \
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

$(OBJ_DIR)/%.o: src/http_parsing/%.cpp | $(OBJ_DIR)
	$(CC) $(FLAGS) -Iinclude -c $< -o $@

$(OBJ_DIR):
	mkdir -p $(OBJ_DIR)

clean:
	rm -rf $(OBJ_DIR)

fclean: clean
	rm -rf $(NAME)

re: fclean all

# Test target
TEST_NAME = test_config
TEST_OBJS = $(OBJ_DIR)/test_config.o

test: $(TEST_NAME)
	./$(TEST_NAME)

$(TEST_NAME): $(filter-out $(OBJ_DIR)/main.o, $(OBJS)) $(TEST_OBJS)
	$(CC) $(FLAGS) $(filter-out $(OBJ_DIR)/main.o, $(OBJS)) $(TEST_OBJS) -o $(TEST_NAME)

$(OBJ_DIR)/test_config.o: tests/test_config.cpp | $(OBJ_DIR)
	$(CC) $(FLAGS) -Iinclude -c $< -o $@

fclean_test:
	rm -rf $(TEST_NAME)

.PHONY: all clean fclean re test fclean_test
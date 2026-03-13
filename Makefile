# **************************************************************************** #
#                                                                              #
#                                                         :::      ::::::::    #
#    Makefile                                           :+:      :+:    :+:    #
#                                                     +:+ +:+         +:+      #
#    By: sravizza <sravizza@student.42lausanne.c    +#+  +:+       +#+         #
#                                                 +#+#+#+#+#+   +#+            #
#    Created: 2025/02/11 09:56:12 by sravizza          #+#    #+#              #
#    Updated: 2025/11/19 13:31:20 by sravizza         ###   ########.fr        #
#                                                                              #
# **************************************************************************** #

################################################################################
##								DIRECTORIES									  ##

SRC_DIR			= src
OBJ_DIR			= obj
INCL_DIR		= include

CGI_DIR			= handle_cgi
CLIENT_DIR		= handle_client
METH_DIR		= handle_methods
HTTP_PARS_DIR	= http_parsing
INIT_DIR		= init
RESP_DIR		= responses


################################################################################
##								  SOURCES									  ##

SRC_MAIN		=	main.cpp \
					epoll.cpp \
					timeout.cpp 

SRC_CGI			=	CGI.cpp \
					cgiHandling.cpp \
					cgiInit.cpp \
					lexCGIOutput.cpp \
					parseCGIOutput.cpp

SRC_CLIENT		=	Client.cpp \
					handleRequest.cpp \
					handleClient.cpp 

SRC_METH		=	dispatcher.cpp \
					methods.cpp

SRC_HTTP_PARS 	=	createTempFile.cpp \
					httpLexer.cpp \
					lexerUtils.cpp \
					parsingMethods.cpp \
					Request.cpp 

SRC_INIT		=	Config.cpp \
					configFile.cpp \
					init_sockets_epoll.cpp \
					Location.cpp \
					Server.cpp \
					Webserv.cpp

SRC_RESP		=	Response.cpp \
					error_response.cpp \
					method_response.cpp \
					raw_response.cpp 



SRC				= 	$(SRC_MAIN) \
					$(addprefix $(CGI_DIR)/, $(SRC_CGI)) \
					$(addprefix $(CLIENT_DIR)/, $(SRC_CLIENT)) \
					$(addprefix $(METH_DIR)/, $(SRC_METH)) \
					$(addprefix $(HTTP_PARS_DIR)/, $(SRC_HTTP_PARS)) \
					$(addprefix $(INIT_DIR)/, $(SRC_INIT)) \
					$(addprefix $(RESP_DIR)/, $(SRC_RESP)) 


################################################################################
##								 ARGUMENTS									  ##


NAME	= webserv
CC		= g++
CFLAGS	= -Wall -Werror -Wextra -I$(INCL_DIR)
OBJ		= $(addprefix $(OBJ_DIR)/, $(SRC:.cpp=.o))
RM		= rm -f
AR		= ar -rcs


################################################################################
##								   RULES									  ##

$(NAME): $(OBJ) 
	$(CC) $(CFLAGS) $(OBJ) -o $(NAME)

$(OBJ_DIR)/%.o: $(SRC_DIR)/%.cpp | create_obj_dirs
	$(CC) $(CFLAGS) -c $< -o $@

$(OBJ_DIR)/%.o: $(CGI_DIR)/%.cpp | create_obj_dirs
	$(CC) $(CFLAGS) -c $< -o $@

$(OBJ_DIR)/%.o: $(CLIENT_DIR)/%.cpp | create_obj_dirs
	$(CC) $(CFLAGS) -c $< -o $@

$(OBJ_DIR)/%.o: $(METH_DIR)/%.cpp | create_obj_dirs
	$(CC) $(CFLAGS) -c $< -o $@

$(OBJ_DIR)/%.o: $(HTTP_PARS_DIR)/%.cpp | create_obj_dirs
	$(CC) $(CFLAGS) -c $< -o $@

$(OBJ_DIR)/%.o: $(INIT_DIR)/%.cpp | create_obj_dirs
	$(CC) $(CFLAGS) -c $< -o $@

$(OBJ_DIR)/%.o: $(RESP_DIR)/%.cpp | create_obj_dirs
	$(CC) $(CFLAGS) -c $< -o $@




create_obj_dirs:
	mkdir -p	$(OBJ_DIR)/$(CGI_DIR) \
				$(OBJ_DIR)/$(CLIENT_DIR) \
				$(OBJ_DIR)/$(METH_DIR) \
				$(OBJ_DIR)/$(HTTP_PARS_DIR) \
				$(OBJ_DIR)/$(INIT_DIR) \
				$(OBJ_DIR)/$(RESP_DIR) 

	
################################################################################
##								   COMMANDS									  ##

all: $(NAME)
	echo $(NAME) "compiled"
clean:
	rm -rf $(OBJ_DIR)
	echo $(NAME) "obj removed"

fclean: clean
	$(RM) $(NAME)
	echo $(NAME) "removed"

re: fclean all

debug: CFLAGS += -g
debug: re
	echo $(NAME) "compiled in debug mode"

valgrind: CFLAGS += -g
valgrind: re
	$(VALGRIND) $(VFLAGS) $(VSUPP) ./$(NAME)

.SILENT:

.PHONY: all clean fclean re debug
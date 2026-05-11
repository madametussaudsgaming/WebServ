# **************************************************************************** #
#                                                                              #
#                                                         :::      ::::::::    #
#    Makefile                                           :+:      :+:    :+:    #
#                                                     +:+ +:+         +:+      #
#    By: alechin <alechin@student.42kl.edu.my>      +#+  +:+       +#+         #
#                                                 +#+#+#+#+#+   +#+            #
#    Created: 2026/03/17 13:12:01 by alechin           #+#    #+#              #
#    Updated: 2026/05/11 13:44:35 by alechin          ###   ########.fr        #
#                                                                              #
# **************************************************************************** #

NAME = webserv
CXX = c++
FSAN = -fsanitize=address
CXXFLAGS = -Wall -Wextra -Werror -std=c++98 -g3 -Iincludes $(FSAN)
RM = rm -rf

OBJECT_DIRECTORY = object

SOURCE = \
	src/Error.cpp					\
	src/Utilitys.cpp 				\
	src/Request.cpp					\
	src/Tcp.cpp						\
	src/CGI.cpp						\
	src/ConfigParser.cpp			\
	src/handlePOST.cpp				\
	src/handleDELETE.cpp			\
	src/handleGET.cpp				\
	webserv.cpp						\

HEADER = \
	includes/Request.hpp		\
	includes/Webserv.hpp		\
	includes/Utilitys.hpp		\
	includes/CGI.hpp			\
	includes/Config.hpp			\

OBJECT = $(addprefix $(OBJECT_DIRECTORY)/, $(SOURCE:.cpp=.o))

DEFAULT := \033[1;39m
RESET := \033[0m
GREEN := \033[1;32m
YELLOW := \033[1;33m
CYAN := \033[1;36m

all: $(NAME)

$(NAME): $(OBJECT)
	@printf "\n$(YELLOW)Compiling webserver...$(RESET)\n"
	@$(CXX) $(CXXFLAGS) $(OBJECT) -o $(NAME)
	@printf "\n$(GREEN)Successfully compiled our new webserver!$(RESET)\n"

$(OBJECT_DIRECTORY)/%.o: %.cpp
	@mkdir -p $(dir $@)
	@echo "$(YELLOW)Compiling $<...$(RESET)"
	@$(CXX) $(CXXFLAGS) -c $< -o $@
	
%.o: %.cpp $(HEADER)
	@$(CXX) $(CXXFLAGS) -c $< -o $@

clean:
	@$(RM) $(OBJECT_DIRECTORY)
	@printf "\n$(CYAN)Cleaning object files...$(RESET)\n"

fclean: clean
	@clear
	@$(RM) $(NAME)
	@printf "\n$(GREEN)Removing binary...$(RESET)\n"

re: fclean all

.PHONY: all clean fclean re
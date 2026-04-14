# **************************************************************************** #
#                                                                              #
#                                                         :::      ::::::::    #
#    Makefile                                           :+:      :+:    :+:    #
#                                                     +:+ +:+         +:+      #
#    By: rpadasia <ryanpadasian@gmail.com>          +#+  +:+       +#+         #
#                                                 +#+#+#+#+#+   +#+            #
#    Created: 2026/03/17 13:12:01 by alechin           #+#    #+#              #
#    Updated: 2026/04/14 14:16:09 by rpadasia         ###   ########.fr        #
#                                                                              #
# **************************************************************************** #

NAME = webserv
CXX = c++
FSAN = -fsanitize=address
CXXFLAGS = -Wall -Wextra -Werror -std=c++98 -g3 -Iincludes $(FSAN)
RM = rm -rf

SOURCE = \
	src/Error.cpp					\
	src/Utilitys.cpp 				\
	src/Request.cpp					\
	src/Tcp.cpp						\
	src/ConfigParser.cpp			\
	src/tcp/handle/handlePOST.cpp	\
	src/tcp/handle/handleDELETE.cpp	\
	webserv.cpp						\


HEADER = \
	includes/Request.hpp		\
	includes/Webserv.hpp		\
	includes/Utilitys.hpp		\

OBJECT = $(SOURCE:.cpp=.o)

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

%.o: %.cpp $(HEADER)
	@$(CXX) $(CXXFLAGS) -c $< -o $@

clean:
	@printf "\n$(CYAN)Cleaning object files...$(RESET)\n"
	@$(RM) $(OBJECT)

fclean: clean
	@printf "\n$(GREEN)Removing binary...$(RESET)\n"
	@$(RM) $(NAME)

re: fclean all

.PHONY: all clean fclean re
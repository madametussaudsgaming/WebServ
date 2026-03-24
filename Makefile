# **************************************************************************** #
#                                                                              #
#                                                         :::      ::::::::    #
#    Makefile                                           :+:      :+:    :+:    #
#                                                     +:+ +:+         +:+      #
#    By: alechin <alechin@student.42kl.edu.my>      +#+  +:+       +#+         #
#                                                 +#+#+#+#+#+   +#+            #
#    Created: 2026/03/17 13:12:01 by alechin           #+#    #+#              #
#    Updated: 2026/03/17 15:06:58 by alechin          ###   ########.fr        #
#                                                                              #
# **************************************************************************** #

NAME = webserv
CXX = c++
FSAN = -fsanitize=address
CXXFLAGS = -Wall -Wextra -Werror -std=c++98 -g3 $(FSAN)
RM = rm -rf

SOURCE = \

HEADER = \

OBJECT = $(SOURCE:.cpp=.o)

DEFAULT := \033[1;39m
RESET := \033[0m
GREEN := \033[1;32m
YELLOW := \033[1;33m
CYAN := \033[1;36m

all: $(NAME)

$(NAME): $(OBJECT)
	@printf "\n$(YELLOW)Compiling module...$(RESET)\n"
	@$(CXX) $(CXXFLAGS) $(OBJECT) -o $(NAME)
	@printf "\n$(GREEN)Successfully compiled module!$(RESET)\n"

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
# **************************************************************************** #
#                                                                              #
#                                                         :::      ::::::::    #
#    Makefile                                           :+:      :+:    :+:    #
#                                                     +:+ +:+         +:+      #
#    By: anemet <anemet@student.42luxembourg.lu>    +#+  +:+       +#+         #
#                                                 +#+#+#+#+#+   +#+            #
#    Created: 2025/10/21 19:46:34 by anemet            #+#    #+#              #
#    Updated: 2025/12/17 21:53:58 by anemet           ###   ########.fr        #
#                                                                              #
# **************************************************************************** #


# Directories
SRCDIR = src
OBJDIR = obj
INCDIR = inc

# Compiler
CXX = g++

# Compiler flags
# make DEBUG=1 to compile for debugging enabled
CXXFLAGS = -Wall -Wextra -Werror -std=c++98 -fPIE -I $(INCDIR)

ifeq ($(DEBUG),1)
    CXXFLAGS += -g -O0 -DDEBUG
else
    CXXFLAGS += -O2
endif

# The name of the executable
NAME = webserv

# Source files
SRCS = main.cpp \
        CGI.cpp \
        Config.cpp \
        Connection.cpp \
        Request.cpp \
        Response.cpp \
        Router.cpp \
        Server.cpp \
        Utils.cpp


# Object files in obj directory
OBJS = $(addprefix $(OBJDIR)/, $(SRCS:.cpp=.o))


# Default target
all: $(OBJDIR) $(NAME)

# Create object directory if it doesn't exist
$(OBJDIR):
	mkdir -p $(OBJDIR)

# Rule to link the object files and create the executable
$(NAME): $(OBJS)
	$(CXX) $(CXXFLAGS) -o $(NAME) $(OBJS)

# Rule to compile a .cpp file into a .o file
$(OBJDIR)/%.o: $(SRCDIR)/%.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

# Clean target to remove object files
clean:
	rm -rf $(OBJDIR)

# Fclean target to remove object files and the executable
fclean: clean
	rm -f $(NAME)

# Re target to rebuild everything
re: fclean all

.PHONY: all clean fclean re

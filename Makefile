NAME		=	exe

SRC_DIR		=	src
OBJ_DIR		=	obj
INCLUDE_DIR	=	include

INCLUDES	=	-I $(INCLUDE_DIR)
SRCDIRS		=	$(shell find $(SRC_DIR) -type d)
INCLUDES	+=	$(addprefix -I ,$(SRCDIRS))

CC			=	c++
CFLAGS		=	-Wall -Wextra -Werror -std=c++98
RM			=	rm

SRCS		=	$(SRC_DIR)/EchoServer.cpp \
					$(SRC_DIR)/Socket.cpp \
					$(SRC_DIR)/Epoll.cpp
OBJS		=	$(SRCS:$(SRC_DIR)/%.cpp=$(OBJ_DIR)/%.o)
DEPS		=	$(SRCS:$(SRC_DIR)/%.cpp=$(OBJ_DIR)/%.d)
.PHONY: $(DEPS)

all:			$(NAME)

$(NAME):		$(OBJS)
				$(CC) $(OBJS) $(CFLAGS) -o $(NAME)

$(OBJ_DIR)/%.o:	$(SRC_DIR)/%.cpp
				@mkdir -p $$(dirname $@)
				$(CC) $(CFLAGS) $(INCLUDES) -MMD -MP -c $< -o $@

clean:
				$(RM) -rf $(OBJS) $(OBJ_DIR)

fclean:			clean
				$(RM) -f $(NAME)

re:				fclean all

.PHONY:			all clean fclean re
-include $(DEPS)

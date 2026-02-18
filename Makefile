NAME			:= webserver
NAME_DEBUG		:= $(NAME)_debug
NAME_TESTS		:= $(NAME)_tests
CXX				:= c++
CXX_FLAGS		:= -Wall -Werror -Wextra -std=c++98 -MMD -MP
TEST_FLAGS		:= -lgtest -lgtest_main -lpthread
LDFLAGS			:=

SRC_DIR			:= srcs
OBJ_DIR			:= obj

SERVER_DIR		:= server
SOCKETS_DIR		:= sockets
PARSER_DIR		:= parser
STRVIEW_DIR		:= strView
MODULES			:= $(SERVER_DIR) \
				   $(SOCKETS_DIR) \
				   $(PARSER_DIR) \
				   $(STRVIEW_DIR)

# Capture filter argument (e.g., "make debug_test Token")
FILTER_ARG		:= $(word 2,$(MAKECMDGOALS))

# Create wildcard pattern if filter argument exists
ifneq ($(FILTER_ARG),)
	FILTER		:= *$(FILTER_ARG)*
endif

# Includes
INCLUDE_DIRS	:= $(SRC_DIR) $(addprefix $(SRC_DIR)/,$(MODULES))
INCLUDE_FLAGS	:= $(addprefix -I,$(INCLUDE_DIRS))

# Sources
SRCS_MAIN		:= main.cpp
SRCS_SERVER		:= Server.cpp
SRCS_SOCKET		:= ASocket.cpp \
				   Listening.cpp \
				   Connection.cpp
SRCS_PARSER		:= Token.cpp
SRCS_STRVIEW	:= StrView.cpp

SRCS_CORE		:= $(SRCS_SERVER:%.cpp=$(SERVER_DIR)/%.cpp) \
				   $(SRCS_SOCKET:%.cpp=$(SOCKETS_DIR)/%.cpp) \
				   $(SRCS_PARSER:%.cpp=$(PARSER_DIR)/%.cpp) \
				   $(SRCS_STRVIEW:%.cpp=$(STRVIEW_DIR)/%.cpp)

# Srcs config (for both test and debug_test)
ifneq (,$(filter test debug_test,$(MAKECMDGOALS)))
	SRCS_TESTS	:= $(foreach src,$(SRCS_CORE),$(dir $(src))test_$(notdir $(src)))
	SRCS		:= $(SRCS_CORE) $(SRCS_TESTS)
else
	SRCS		:= $(SRCS_MAIN) $(SRCS_CORE)
endif

# Debug config (for both debug and debug_test)
ifneq (,$(filter debug debug_test,$(MAKECMDGOALS)))
	CXX_FLAGS	+= -g
endif

# Regular debug settings
ifeq ($(MAKECMDGOALS),debug)
	NAME		:= $(NAME_DEBUG)
	OBJ_DIR		:= $(OBJ_DIR)_debug
endif

# Tests config (for both test and debug_test)
ifneq (,$(filter test debug_test,$(MAKECMDGOALS)))
	LDFLAGS		+= $(TEST_FLAGS)
	NAME		:= $(NAME_TESTS)
	OBJ_DIR		:= $(OBJ_DIR)_tests
endif

OBJS			:= $(SRCS:%.cpp=$(OBJ_DIR)/%.o)
DEPS			:= $(OBJS:.o=.d)

# Rules
all: $(NAME)
debug: $(NAME)
test: $(NAME)
	./$(NAME)

debug_test: $(NAME)
	@if [ -z "$(FILTER)" ]; then \
		gdb ./$(NAME); \
	else \
		echo "Running tests matching: $(FILTER)"; \
		gdb --args ./$(NAME) --gtest_filter=$(FILTER); \
	fi

$(NAME): $(OBJS)
	$(CXX) $(CXX_FLAGS) $(OBJS) $(LDFLAGS) -o $(NAME)

$(OBJ_DIR)/%.o: $(SRC_DIR)/%.cpp
	@mkdir -p $(dir $@)
	$(CXX) $(CXX_FLAGS) $(INCLUDE_FLAGS) -c $< -o $@ 

-include $(DEPS)

clean:
	rm -rf obj obj_debug obj_tests

fclean: clean
	rm -f $(NAME) $(NAME_DEBUG) $(NAME_TESTS)

re: fclean all

# Dummy target to prevent make from complaining about filter argument
%:
	@:

.PHONY: all debug test debug_test clean fclean re

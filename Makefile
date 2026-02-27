NAME			:= webserver
NAME_DEBUG		:= $(NAME)_debug
NAME_TESTS		:= $(NAME)_tests
CXX				:= c++
CXX_FLAGS		:= -Wall -Werror -Wextra -std=c++98 -MMD -MP
TEST_FLAGS		:= -lgtest -lgtest_main -lpthread
LDFLAGS			:=
SRC_DIR			:= srcs
OBJ_DIR			:= obj

# Capture filter argument (e.g., "make debug_test Token")
FILTER_ARG		:= $(word 2,$(MAKECMDGOALS))
# Create wildcard pattern if filter argument exists
ifneq ($(FILTER_ARG),)
	FILTER		:= *$(FILTER_ARG)*
endif

# Source Main
SRCS_MAIN		:= main.cpp
# Sources modules (first word is the directory)
SRCS_ENGINE		:= engine Engine.cpp
SRCS_SERVER		:= server Server.cpp
SRCS_SOCKET		:= sockets ASocket.cpp Listening.cpp Connection.cpp
SRCS_PARSER		:= parser Token.cpp ConfParser.cpp Expect.cpp
SRCS_UTILS		:= utils StrView.cpp

SRC_GROUPS		:= SRCS_ENGINE SRCS_SERVER SRCS_SOCKET SRCS_PARSER SRCS_UTILS

define make_paths
$(addprefix $(word 1,$(1))/,$(wordlist 2,$(words $(1)),$(1)))
endef

# Create test file paths from a source group
define make_test_paths
$(addprefix $(word 1,$(1))/test_,$(wordlist 2,$(words $(1)),$(1)))
endef

# Build SRCS_CORE and SRCS_TEST_CORE dynamically from groups
SRCS_CORE		:= $(foreach group,$(SRC_GROUPS),$(call make_paths,$($(group))))
SRCS_TEST_CORE	:= $(foreach group,$(SRC_GROUPS),$(call make_test_paths,$($(group))))

# Extract module directories from source groups
MODULES			:= $(foreach group,$(SRC_GROUPS),$(word 1,$($(group)))) $(ENGINE_DIR) $(UTILS_DIR)

# Includes
INCLUDE_DIRS	:= $(SRC_DIR) $(addprefix $(SRC_DIR)/,$(MODULES))
INCLUDE_FLAGS	:= $(addprefix -I,$(INCLUDE_DIRS))

# Detect build mode from MAKECMDGOALS
IS_DEBUG		:= $(filter debug debug_test,$(MAKECMDGOALS))
IS_TEST			:= $(filter test debug_test,$(MAKECMDGOALS))

# Source configuration
ifdef IS_TEST
	SRCS		:= $(SRCS_CORE) $(SRCS_TEST_CORE)
else
	SRCS		:= $(SRCS_MAIN) $(SRCS_CORE)
endif

# Compiler flags
ifdef IS_DEBUG
	CXX_FLAGS	+= -ggdb -D_GLIBCXX_DEBUG
endif

# Linker flags
ifdef IS_TEST
	LDFLAGS		+= $(TEST_FLAGS)
endif

# Binary name and object directory
ifdef IS_TEST
	NAME		:= $(NAME_TESTS)
	OBJ_DIR		:= $(OBJ_DIR)_tests$(if $(IS_DEBUG),_debug)
else ifdef IS_DEBUG
	NAME		:= $(NAME_DEBUG)
	OBJ_DIR		:= $(OBJ_DIR)_debug
endif

OBJS			:= $(SRCS:%.cpp=$(OBJ_DIR)/%.o)
DEPS			:= $(OBJS:.o=.d)

# Define test run command
define run_tests
	@if [ -z "$(FILTER)" ]; then \
		$(1) ./$(NAME); \
	else \
		echo "Running tests matching: $(FILTER)"; \
		$(1) ./$(NAME) --gtest_filter=$(FILTER); \
	fi
endef

# Rules
all: $(NAME)

debug: $(NAME)

test: $(NAME)
	$(call run_tests,)

debug_test: $(NAME)
	$(call run_tests,gdb --args)

$(NAME): $(OBJS)
	$(CXX) $(CXX_FLAGS) $(OBJS) $(LDFLAGS) -o $(NAME)

$(OBJ_DIR)/%.o: $(SRC_DIR)/%.cpp
	@mkdir -p $(dir $@)
	$(CXX) $(CXX_FLAGS) $(INCLUDE_FLAGS) -c $< -o $@ 

-include $(DEPS)

clean:
	rm -rf obj obj_debug obj_tests obj_tests_debug

fclean: clean
	rm -f $(NAME) $(NAME_DEBUG) $(NAME_TESTS)

re: fclean all

# Dummy target only for the filter argument (not for test/debug_test themselves)
ifneq ($(FILTER_ARG),)
$(FILTER_ARG):
	@:
endif

.PHONY: all debug test debug_test clean fclean re

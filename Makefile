###############################################################
#
#                          Makefile
#
#    Maintenance targets:
#    - all: (default target) Compile all executables.
#    - clean: Clean out all compiled object and executables.
#    - test: Compile all executables and run all tests.
#    - valgrind-test: Compile all executables and run all
#      tests with valgrind.
#
###############################################################

############## Variables ###############
# Executables to build using "make all".
EXECUTABLES = proxy tcpclient

# Tests to build using "make test".
TESTS = test_logger

# Custom headers (.h files) in your directory.
INCLUDES =

# Compilor.
CC= gcc

# Updating include path.
IFLAGS = -I.

# Compile flags.
# -g3: Debugging information settings.
# -O0: Optimization settings.
# -std=gnu99: c standard settings.
# -Wall -Wextra -Werror -Wfatal-errors -pedantic: Max out warnings.
# $(IFLAGS): Include path settings.
CFLAGS = -g3 -O0 -std=gnu99 -Wall -Wextra -Werror -Wfatal-errors -pedantic $(IFLAGS)

# Linking flags, used in the linking step.
# Set debugging information and update linking path.
LDFLAGS = -g3

# Libraries needed for any of the programs that will be linked
# -lnsl: network service library.
LDLIBS = -lnsl

############### Rules ###############
.PHONY: all clean test valgrind-test

# 'make all' will build all executables
# Note that "all" is the default target that make will build
# if nothing is specifically requested
all: $(EXECUTABLES)

# 'make clean' will remove all object and executable files
clean:
	rm -f $(EXECUTABLES) $(TESTS) *.o

# `make test` will build all executables and tests, then run tests.
test: all $(TESTS)
	for test in $(TESTS); do echo $$test && ./$$test || exit 1; done

# `make valgrind-test` will build all executables and tests, then run tests with
# valgrind.
valgrind-test: all
	for test in $(TESTS); do \
    echo $$test && valgrind --leak-check=full ./$$test || exit 1; \
    done

# Compile step (.c files -> .o files)
# To get *any* .o file, compile its .c file with the following rule.
%.o:%.c $(INCLUDES)
	$(CC) $(CFLAGS) -c $<

# Linking step (.o -> executable program)
# Each executable depends on one or more .o files.
# Those .o files are linked together to build the corresponding
# executable.
proxy: proxy.o logger.o
	$(CC) $(LDFLAGS) $^ -o $@ $(LDLIBS)

test_logger: test_logger.o logger.o
	$(CC) $(LDFLAGS) $^ -o $@ $(LDLIBS)

tcpclient: tcpclient.o
	$(CC) $(LDFLAGS) $^ -o $@ $(LDLIBS)
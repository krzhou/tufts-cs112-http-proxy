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
# Port that the proxy listens on.
PORT = 9160

# Executables to build using "make all".
EXECUTABLES = proxy

# Tests to build using "make test".
TESTS = test_logger test_sock_buf test_cache

# Custom headers (.h files) in your directory.
INCLUDES = cache.h http_utils.h logger.h sock_buf.h

# Compilor.
CC= gcc

# Updating include path.
IFLAGS = -I.

# Compile flags.
# -g3: Debugging information settings.
# -O0: Optimization settings.
# -std=gnu99: c standard settings.
# -Wall -Wextra -pedantic: Max out warnings.
# $(IFLAGS): Include path settings.
CFLAGS = -g3 -O0 -std=gnu99 -Wall -Wextra -pedantic $(IFLAGS)

# Linking flags, used in the linking step.
# Set debugging information and update linking path.
LDFLAGS = -g3

# Libraries needed for any of the programs that will be linked
# -lnsl: network service library.
# -lssl: secure socket layer library from OpenSSL.
# -lcrypto: crypto library from OpenSSL.
LDLIBS = -lnsl -lssl -lcrypto

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

# `make integration-test` will build all executables, then run integration tests
# using Python unittest.
integration-test: all
	python3 test_proxy_default.py $(PORT) || exit 1
	python3 test_proxy_ssl_interception.py $(PORT) || exit 1

bench: all
	python3 bench_proxy_default.py $(PORT)
	python3 bench_proxy_ssl_interception.py $(PORT)

# Compile step (.c files -> .o files)
# To get *any* .o file, compile its .c file with the following rule.
%.o:%.c $(INCLUDES)
	$(CC) $(CFLAGS) -c $<

# Linking step (.o -> executable program)
# Each executable depends on one or more .o files.
# Those .o files are linked together to build the corresponding
# executable.
proxy: proxy.o logger.o cache.o sock_buf.o http_utils.o
	$(CC) $(LDFLAGS) $^ -o $@ $(LDLIBS)

test_logger: test_logger.o logger.o
	$(CC) $(LDFLAGS) $^ -o $@ $(LDLIBS)

test_sock_buf: test_sock_buf.o sock_buf.o logger.o
	$(CC) $(LDFLAGS) $^ -o $@ $(LDLIBS)

test_cache: test_cache.o cache.o logger.o
	$(CC) $(LDFLAGS) $^ -o $@ $(LDLIBS)

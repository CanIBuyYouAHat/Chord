# Compiler
CC = gcc

# Compiler flags
CFLAGS = -Wall -Wextra -std=c99 -I.

# Linker flags for OpenSSL
LDFLAGS = -lssl -lcrypto

# Source files
SRCS = example_hash.c hash.c

# Object files
OBJS = $(SRCS:.c=.o)

# Executable name
EXEC = example

# Default target
all: $(EXEC)

# Build the executable
$(EXEC): $(OBJS)
	$(CC) $(OBJS) -o $@ $(LDFLAGS)

# Compile source files to object files
%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

# Clean target to remove compiled files
clean:
	rm -f $(OBJS) $(EXEC)

# Phony targets
.PHONY: all clean


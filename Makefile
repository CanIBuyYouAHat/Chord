# Compiler
CC = gcc

# Compiler flags
CFLAGS = -Ichord -Ihash -Iht -Iprotobuf -Wall -Wextra

# Linker flags for OpenSSL
LDFLAGS = -L/usr/local/opt/openssl/lib -lssl -lcrypto -lprotobuf-c

# Protobuf source and object files
PROTO_SRCS = protobuf/chord.pb-c.c
PROTO_OBJS = $(PROTO_SRCS:.c=.o)

# Hash source and object files
HASH_SRCS = hash/hash.c
HASH_OBJS = $(HASH_SRCS:.c=.o)

# Hashtable source and object files
HT_SRCS = ht/ht.c
HT_OBJS = $(HT_SRCS:.c=.o)

# Chord source and object files
CHORD_SRCS = chord/chord.c
CHORD_OBJS = $(CHORD_SRCS:.c=.o)

# Main source and object files
MAIN_SRCS = main.c
MAIN_OBJS = $(MAIN_SRCS:.c=.o)

# All object files
OBJS = $(CHORD_OBJS) $(PROTO_OBJS) $(HASH_OBJS) $(HT_OBJS) $(MAIN_OBJS)

# Executables
TARGET = main

# Default target
all: $(TARGET)

# Build the main executable
$(TARGET): $(OBJS)
	$(CC) $(OBJS) -o $(TARGET) $(LDFLAGS)

# Compile rules
%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

# Clean target
clean:
	rm -f $(OBJS) $(TARGET)

# Phony targets
.PHONY: all clean


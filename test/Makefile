CC = gcc
CFLAGS = -I../chord -I../protobuf -I../hash -Wall -Wextra 
LDFLAGS = -lprotobuf-c -lpthread -L/usr/local/opt/openssl/lib -lssl -lcrypto

SRC = test_client.c ../chord/chord.c ../protobuf/chord.pb-c.c ../hash/hash.c
OBJ = $(SRC:.c=.o)
TARGET = test_program

all: $(TARGET)

$(TARGET): $(OBJ)
	$(CC) $(OBJ) -o $(TARGET) $(LDFLAGS)

clean:
	rm -f $(OBJ) $(TARGET)

.PHONY: all clean


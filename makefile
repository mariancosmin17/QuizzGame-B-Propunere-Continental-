CC = gcc
CFLAGS = -Wall -g -I/usr/include/libxml2

LDFLAGS = -lxml2 -lpthread

SERVER_TARGET = server
CLIENT_TARGET = client
SERVER_SRC = server.c
CLIENT_SRC = client.c

all: $(SERVER_TARGET) $(CLIENT_TARGET)

$(SERVER_TARGET): $(SERVER_SRC)
	$(CC) $(CFLAGS) -o $(SERVER_TARGET) $(SERVER_SRC) $(LDFLAGS)

$(CLIENT_TARGET): $(CLIENT_SRC)
	$(CC) $(CFLAGS) -o $(CLIENT_TARGET) $(CLIENT_SRC)

clean:
	rm -f $(SERVER_TARGET) $(CLIENT_TARGET)

.PHONY: all clean


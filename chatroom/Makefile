CC ?= gcc
CFLAGS ?= -Wall -Wextra -Wpedantic -Wconversion -Wshadow -std=c11 -Iincludes
LDFLAGS ?= -pthread

SERVER_BIN = server
CLIENT_BIN = client

.PHONY: all clean

all: $(SERVER_BIN) $(CLIENT_BIN)

$(SERVER_BIN): server.c
	$(CC) $(CFLAGS) -o $@ $< $(LDFLAGS)

$(CLIENT_BIN): client.c
	$(CC) $(CFLAGS) -o $@ $< $(LDFLAGS)

clean:
	rm -f $(SERVER_BIN) $(CLIENT_BIN) *.o

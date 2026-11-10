CC ?= gcc
CFLAGS ?= -Wall -Wextra -Wpedantic -Wconversion -Wshadow -Werror -std=c11 -Iincludes
LDFLAGS ?= -pthread

SERVER_BIN = server
CLIENT_BIN = client
TEST_BIN = test_concurrency

.PHONY: all clean test

all: $(SERVER_BIN) $(CLIENT_BIN)

$(SERVER_BIN): server.c
	$(CC) $(CFLAGS) -o $@ $< $(LDFLAGS)

$(CLIENT_BIN): client.c
	$(CC) $(CFLAGS) -o $@ $< $(LDFLAGS)

$(TEST_BIN): tests/test_concurrency.c
	$(CC) $(CFLAGS) -o $@ $< $(LDFLAGS)

test: all $(TEST_BIN)
	./$(TEST_BIN)

clean:
	rm -f $(SERVER_BIN) $(CLIENT_BIN) $(TEST_BIN) *.o

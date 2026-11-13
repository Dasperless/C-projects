CC ?= gcc
CFLAGS ?= -Wall -Wextra -Wpedantic -Wconversion -Wshadow -Werror -std=c11 -Iincludes
LDFLAGS ?= -pthread

SERVER_BIN = server
CLIENT_BIN = client
TEST_BINS = test_fileshare test_logger test_protocol test_chat_features test_chat_logging test_file_transfer test_concurrency

.PHONY: all clean test

all: $(SERVER_BIN) $(CLIENT_BIN)

$(SERVER_BIN): server.c protocol.c logger.c fileshare.c
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

$(CLIENT_BIN): client.c protocol.c fileshare.c
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

test_fileshare: tests/test_fileshare.c fileshare.c
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

test_protocol: tests/test_protocol.c protocol.c
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

test_chat_features: tests/test_chat_features.c protocol.c
	$(CC) $(CFLAGS) -Itests -o $@ $^ $(LDFLAGS)

test_chat_logging: tests/test_chat_logging.c protocol.c
	$(CC) $(CFLAGS) -Itests -o $@ $^ $(LDFLAGS)

test_file_transfer: tests/test_file_transfer.c protocol.c fileshare.c
	$(CC) $(CFLAGS) -Itests -o $@ $^ $(LDFLAGS)

test_concurrency: tests/test_concurrency.c protocol.c
	$(CC) $(CFLAGS) -Itests -o $@ $^ $(LDFLAGS)

test: all $(TEST_BINS)
	./test_fileshare
	./test_logger
	./test_protocol
	./test_chat_features ./$(SERVER_BIN)
	./test_chat_logging ./$(SERVER_BIN)
	./test_file_transfer ./$(SERVER_BIN)
	./test_concurrency ./$(SERVER_BIN)

clean:
	rm -f $(SERVER_BIN) $(CLIENT_BIN) $(TEST_BINS) *.o

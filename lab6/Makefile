CC = gcc
CFLAGS = -Wall -Wextra -Werror

BIN_PIPE = pipe_demo
BIN_FIFO_WRITER = fifo_writer
BIN_FIFO_READER = fifo_reader

all: $(BIN_PIPE) $(BIN_FIFO_WRITER) $(BIN_FIFO_READER)

$(BIN_PIPE): pipe_demo.c
	$(CC) $(CFLAGS) -o $@ $<

$(BIN_FIFO_WRITER): fifo_writer.c
	$(CC) $(CFLAGS) -o $@ $<

$(BIN_FIFO_READER): fifo_reader.c
	$(CC) $(CFLAGS) -o $@ $<

clean:
	rm -f $(BIN_PIPE) $(BIN_FIFO_WRITER) $(BIN_FIFO_READER)

.PHONY: all clean


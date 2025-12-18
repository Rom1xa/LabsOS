CC = gcc
CFLAGS = -Wall -Wextra -Werror -pthread

SENDER = sender
RECEIVER = receiver

all: $(SENDER) $(RECEIVER)

$(SENDER): sender.c
	$(CC) $(CFLAGS) -o $@ $<

$(RECEIVER): receiver.c
	$(CC) $(CFLAGS) -o $@ $<

clean:
	rm -f $(SENDER) $(RECEIVER)

.PHONY: all clean

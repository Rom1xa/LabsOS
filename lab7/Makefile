CC = gcc
CFLAGS = -Wall -Wextra -Werror

SENDER = shmem_sender
RECEIVER = shmem_receiver

all: $(SENDER) $(RECEIVER)

$(SENDER): shmem_sender.c
	$(CC) $(CFLAGS) -o $@ $<

$(RECEIVER): shmem_receiver.c
	$(CC) $(CFLAGS) -o $@ $<

clean:
	rm -f $(SENDER) $(RECEIVER)
	ipcrm -M 4660 >/dev/null 2>&1 || true
	rm -f /dev/shm/sem.lab7_prod_lock

.PHONY: all clean


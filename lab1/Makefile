CFLAGS  := -Wall -Wextra

BIN     := mycat mygrep

all: $(BIN)

mycat: mycat.c
	gcc $(CFLAGS) $< -o $@

mygrep: mygrep.c
	gcc $(CFLAGS) $< -o $@

.PHONY: clean 

clean:
	rm -f $(BIN)


src = $(wildcard *.c)
obj = $(src:.c=.o)
CC = gcc
LDFLAGS = -lnsl

a.out: $(obj)
	$(CC) -o $@ $^ $(LDFLAGS) -L/usr/lib -lssl -lcrypto

.PHONY: clean
clean:
	rm -f $(obj) a.out


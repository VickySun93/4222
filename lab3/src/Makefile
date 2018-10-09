CC=gcc
CFLAGS=-I. -Werror -Wall -pthread

all:  tunneld mytunnel

tunneld : tunneld.o
	$(CC) $(CFLAGS) -o $@ $<

mytunnel: mytunnel.o
	$(CC) $(CFLAGS) -o $@ $<
o: %.c
	$(CC) -c -o $@ $< $(CFLAGS)

clean:
	rm -f *.o  mytunnel  tunneld 
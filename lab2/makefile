CC=gcc
CFLAGS=-I. -Werror -Wall -pthread

all:  rfss



rfss: rfss.o
	$(CC) $(CFLAGS) -o $@ $<
o: %.c
	$(CC) -c -o $@ $< $(CFLAGS)

clean:
	rm -f *.o  rfss

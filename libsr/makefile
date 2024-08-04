CC=gcc
CFLAG=-g -O1 -Wall -std=c11
SOURCES= linkedList.c receiver.c sender.c buffer_queue.c sr.c bst.c xorshift.c

OBJECTS = $(SOURCES:%.c=%.o)

%.o: %.c
	$(CC) -c $<

libsr.a: $(OBJECTS)
	ar r $@ $^

clean:
	rm *.o libsr.a
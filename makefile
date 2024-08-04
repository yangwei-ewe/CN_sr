CC=gcc
CFLAG=-g -O1 -Wall -std=c11
# circularLinkList.c
LIBRARYS=libsr.a

%.o: %.c
	$(CC) -c $<

all: sender receiver

libsr.a: 
	cd libsr; make clean; make && cp libsr.a ..

sender: sender_main.o $(LIBRARYS)
	$(CC) $(CFLAG) $^ -o $@ -lssl -lcrypto

receiver: receiver_main.o $(LIBRARYS) 
	$(CC) $(CFLAG) $^ -o $@ -lssl -lcrypto

tmp: tmp.o $(LIBRARYS)
	$(CC) $(CFLAG) $^ -o $@ -lssl -lcrypto

clean:
	rm *.o receiver sender libsr.a; cd libsr ; make clean
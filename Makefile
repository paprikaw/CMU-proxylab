# Makefile for Proxy Lab 
#
# You may modify this file any way you like (except for the handin
# rule). You instructor will type "make" on your specific Makefile to
# build your proxy from sources.

CC = gcc
CFLAGS = -g
LDFLAGS = -lpthread

all: proxy

url_parser.o: url_parser.c url_parser.h
	$(CC) $(CFLAGS) -c url_parser.c

sbuf.o: sbuf.c sbuf.h
	$(CC) $(CFLAGS) -c sbuf.c

csapp.o: csapp.c csapp.h
	$(CC) $(CFLAGS) -c csapp.c

proxy.o: proxy.c csapp.h url_parser.h sbuf.h
	$(CC) $(CFLAGS) -c proxy.c

proxy: proxy.o csapp.o url_parser.o sbuf.o
	$(CC) $(CFLAGS) proxy.o csapp.o url_parser.o sbuf.o -o proxy $(LDFLAGS)

# Creates a tarball in ../proxylab-handin.tar that you can then
# hand in. DO NOT MODIFY THIS!
handin:
	(make clean; cd ..; tar cvf $(USER)-proxylab-handin.tar proxylab-handout --exclude tiny --exclude nop-server.py --exclude proxy --exclude driver.sh --exclude port-for-user.pl --exclude free-port.sh --exclude ".*")

clean:
	rm -f *~ *.o proxy core *.tar *.zip *.gzip *.bzip *.gz


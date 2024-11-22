CC = gcc
CFLAGS = -g -Wall
LDFLAGS = -lpthread

all: proxy

http.o: http.c http.h
	$(CC) $(CFLAGS) -c http.c

error.o: error.c error.h
	$(CC) $(CFLAGS) -c error.c

io.o: io.c io.h
	$(CC) $(CFLAGS) -c io.c

proxy.o: proxy.c proxy.h
	$(CC) $(CFLAGS) -c proxy.c

proxy: proxy.o error.o io.o http.o
	$(CC) $(CFLAGS) error.o io.o http.o proxy.o -o proxy $(LDFLAGS)

clean:
	rm -f *~ *.o proxy core *.tar *.zip *.gzip *.bzip *.gz

CC=gcc
CFLAGS=-I/usr/include/libxml2
LIBS=-lcurl -lxml2
plexamp-control: plexamp-control.c
	$(CC) $(CFLAGS) -o plexamp-control plexamp-control.c $(LIBS)
clean:
	rm plexamp-control

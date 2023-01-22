CC = gcc
CFLAGS = -Wall

client: client.c
	$(CC) $(CFLAGS) client.c -o client

server: server.c
	$(CC) $(CFLAGS) server.c -o server

all: server client

clean:
	rm -f *.o server client

client: client.c
	gcc -Wall client.c -o client
server: server.c
	gcc -Wall server.c -o server
all: server client
clear:
	rm -f *.o server client

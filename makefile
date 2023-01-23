client: inf151868_inf151850_k.c
	gcc -Wall inf151868_inf151850_k.c -o client
server: inf151868_inf151850_s.c
	gcc -Wall inf151868_inf151850_s.c -o server
all: server client
clear:
	rm -f *.o server client

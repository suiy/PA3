all:
	gcc -Wall -o client client.c
	gcc -Wall -o server server.c -lpthread

clean:
	rm client
	rm server

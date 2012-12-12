all:
	gcc -Wall -o client client.c
	gcc -Wall -o FDserver FDserver.c -lpthread
	gcc -Wall -o CAserver CAserver.c -lpthread -lssl -lcrypto

clean:
	-rm client
	-rm FDserver
	-rm CAserver
	-rm ./client1/download_files/*
	-rm ./client2/download_files/*

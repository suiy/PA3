#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <unistd.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <dirent.h> 
#include <pthread.h>
#include <netdb.h>
#include <sys/param.h>
#include <sys/time.h>

/* OpenSSL headers */
#include <openssl/bio.h>
#include <openssl/ssl.h>
#include <openssl/err.h>

#define MAXBUFSIZE 1024
#define MAX_CONNECTS 50


char* c_name;
char* server_ip;
int server_port;
char* file_list;
int client_port;

char* filelist(char* name, char* file_list,char* buf,int port);
int tcp_connect(char* hostname, int port);
void send_file(char* filename,int sock);
void receive_file(char* filename,int sock);
int tcp_listen(int port);

int main(int argc, char *argv[])
{
	
	//Read Command Line and set values
    if (argc != 5) {
        printf("usage: %s <client name> <server ip> <server port #> <list of files from “shared files directory”>\n", argv[0]);
        exit(1);
    }

    c_name = argv[1];
    server_ip = argv[2];
    server_port = atoi(argv[3]);
    file_list=argv[4];

 	// 1. Create socket and connect to server
 	
	int sock,csock,clientsock;
    int clientAddrSize = sizeof(struct sockaddr_in);
    struct sockaddr_in clientAddr;

    sock = tcp_connect(server_ip,server_port);
    srand((unsigned)time(0));
    int ran_num=rand() % 2000;
    client_port=5000+ran_num;
    csock = tcp_listen(client_port);

    //2.send client name and file list to server 
    char buffer[MAXBUFSIZE];
    bzero(buffer,MAXBUFSIZE);
    filelist(c_name,file_list,buffer,client_port);
    printf("client %s is sending file_list\n",c_name);
    send(sock,buffer,sizeof(buffer),0);
    bzero(buffer,MAXBUFSIZE);
    recv(sock,buffer,MAXBUFSIZE,0);
    printf("%s\n",buffer);

    char buf[MAXBUFSIZE];
    char recvbuf[MAXBUFSIZE];
    while (1) {
fd_set rfds;
        struct timeval tv;
        FD_ZERO(&rfds);
        FD_SET(csock, &rfds);
        tv.tv_sec = 0;
        tv.tv_usec = 100000;
        //printf("loop\n");
        if (select(csock + 1, &rfds, NULL, NULL, &tv)) {
            printf("in select 1\n");
            /* if someone is trying to connect, accept() the connection */
            clientsock = accept(csock,
                (struct sockaddr*) &clientAddr,
                (socklen_t*) &clientAddrSize);
            FD_ZERO(&rfds);
            FD_SET(clientsock, &rfds);
            tv.tv_sec = 0;
            tv.tv_usec = 100000;
            if (select(clientsock + 1, &rfds, NULL, NULL, &tv)) {
            bzero(buffer,MAXBUFSIZE);
            bzero(buf,MAXBUFSIZE);
             printf("in select 2\n");
            recv(clientsock,buffer,MAXBUFSIZE,0);
            strncpy(buf,buffer,3);
            if(!strcmp(buf,"Get")){
                send_file(buffer+4,clientsock);
            }
            }
            close(clientsock);
        }
        /* Read command line and store in buffer*/
        bzero(buf,MAXBUFSIZE);
        bzero(buffer,MAXBUFSIZE);
       // printf("> ");
        fd_set fds;
        struct timeval ttv;
        int ioflag = 0;
        ttv.tv_sec = 5; 
        ttv.tv_usec = 0;
        FD_ZERO(&fds);
        FD_SET(0,&fds); //stdin
        select(0+1,&fds,NULL,NULL,&ttv);
        if(FD_ISSET(0,&fds)){
            
            fgets(buf, MAXBUFSIZE, stdin);
            ioflag = 1;
            
            
        }
        if(!ioflag){ //bounce back to start of loop on user io timeout
            continue;
        }
        
        if (strlen(buf) > 0) {
            buf[strlen(buf) - 1] = '\0';
        if(!strcmp(buf,"Exit")){
        	sprintf(buffer,"%s.%s.",c_name,buf);
        	send(sock,buffer,sizeof(buffer),0);
        	close(sock);
        	exit(EXIT_SUCCESS);
        }
        else if(!strcmp(buf,"List")){
        	sprintf(buffer,"%s.%s.",c_name,buf);
        	send(sock,buffer,sizeof(buffer),0);
        }
        else if(!strcmp(buf,"SendMyFilesList")){
		bzero(buffer,MAXBUFSIZE);
    	filelist(c_name,file_list,buffer,client_port);
    	send(sock,buffer,sizeof(buffer),0);
        }
        else if(!strcmp(buf,"")){
            continue;
        }
        else{
        	strncpy(buffer,buf,3);
        	if(!strcmp(buffer,"Get")){
        		bzero(buffer,MAXBUFSIZE);
        		sprintf(buffer,"%s.Get.%s",c_name,buf+4);
        		send(sock,buffer,sizeof(buffer),0);
			}
			else{
				printf("Invalid Command: Exit/List/SendMyFilesList/Get <file>\n");
				continue;
			}
        }
    }
    

        bzero(recvbuf,MAXBUFSIZE);
        bzero(buf,MAXBUFSIZE);
        recv(sock,recvbuf,MAXBUFSIZE,0);
        
        int fdot = strcspn(recvbuf, ".");
		strncpy(buf, recvbuf, fdot);
		if(!strcmp(buf,"Server: receive file_list successfully")){
			printf("%s\n",recvbuf);
		}
		else if(!strcmp(buf,"filelist")){
			printf("Received list of files from server\nFile name || File size KB || File owner\n%s",recvbuf+fdot+1);
		}
		else if(!strcmp(buf,"ip")){
            int sdot = strcspn(recvbuf+fdot+1,"|");
            
            char client_ip[MAXBUFSIZE];
            char filename[MAXBUFSIZE];
            char file_buf[MAXBUFSIZE];
            char cport[MAXBUFSIZE];
            int cp;
            bzero(filename,MAXBUFSIZE);
            strncpy(filename,recvbuf+fdot+1,sdot);
            if(!strcmp(recvbuf+fdot+sdot+2,"File requested is not available")){
                printf("Server: File requested is not available\n");
                continue;
            }

            int adot = strcspn(recvbuf+fdot+sdot+2,".");
            bzero(cport,MAXBUFSIZE);
            strncpy(cport,recvbuf+fdot+sdot+2,adot);
            cp=atoi(cport);
            bzero(client_ip,MAXBUFSIZE);
			strcpy(client_ip,recvbuf+fdot+sdot+adot+3);
            
            
            int currentsock = tcp_connect(client_ip,cp);
            sprintf(file_buf,"Get %s",filename);
            send(currentsock,file_buf,sizeof(file_buf),0);
            receive_file(filename,currentsock);
            close(currentsock);

		}

    }
close(sock);
}
int tcp_connect(char* hostname, int port){
    int sock;
    struct hostent *host;
    struct sockaddr_in server_addr;

    host = gethostbyname(hostname);

    if ((sock = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
        perror("Socket");
        exit(1);
    }

    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(port);
    server_addr.sin_addr = *((struct in_addr*)host->h_addr);
    bzero(&(server_addr.sin_zero), 8);

    if (connect(sock, (struct sockaddr*)&server_addr,
                sizeof(struct sockaddr)) == -1) {
        perror("Connect");
        exit(1);
    }
    
    return sock;
}
int tcp_listen(int port){
    int sock;
    int TRUE = 1;
    
    if ((sock = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
        perror("Socket");
        exit(1);
    }

    if (setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &TRUE, sizeof(int)) == -1) {
        perror("Setsockopt");
        exit(1);
    }

    struct sockaddr_in server_addr;
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(port);
    server_addr.sin_addr.s_addr = INADDR_ANY;
    bzero(&(server_addr.sin_zero), 8);

    if (bind(sock, (struct sockaddr*)&server_addr,
             sizeof(struct sockaddr)) == -1) {
        perror("Unable to bind");
        exit(1);
    }

    if (listen(sock, MAX_CONNECTS) == -1) {
        perror("Listen");
        exit(1);
    }

    return sock;
}
char* filelist(char* name, char* file_list, char* buf,int port){
	char dir[MAXBUFSIZE];
	int size,fdot,sdot;
	char filename[MAXBUFSIZE],buffer[MAXBUFSIZE];
	sdot = 0; fdot=0;
	FILE * f =NULL;
	bzero(buffer,MAXBUFSIZE);
	sprintf(buf,"%s.SendMyFilesList.",name);
	
    //copy information to buffer
    while(sdot<strlen(file_list)){
    	bzero(filename,MAXBUFSIZE);
    	bzero(buffer,MAXBUFSIZE);
    	fdot = strcspn(file_list+sdot, ",");
		strncpy(filename, file_list+sdot, fdot);
		filename[fdot] = '\0';
		bzero(dir,MAXBUFSIZE);
        sprintf(dir,"./%s/shared_files/",c_name);
		strcat(dir,filename);
		f=fopen(dir,"r");
		if (f !=NULL)
    	{
    		fseek(f, 0, SEEK_END); // seek to end of file
			size = ftell(f); // get current file pointer
			fseek(f, 0, SEEK_SET); // seek back to beginning of file
    		fclose(f);
    		sprintf(buffer,"%s||%d||%s||%d.||",filename,size,name,port);
    	}
        else{
            printf("Cannot find file %s\n",filename);
            exit(EXIT_SUCCESS);
        }
		sdot = sdot+fdot +1;
		strcat(buf,buffer);
	}
    return 0;
}
void send_file(char* filename,int sock){
    char dir[MAXBUFSIZE];
    sprintf(dir,"./%s/shared_files/",c_name);
    strcat(dir,filename);
    FILE* fd = fopen(dir, "rb");
    char buffer[MAXBUFSIZE];
    if(fd!=NULL){
    
    printf("sending file %s...\n",filename);
    // Read and send file to other client
    while (!(ferror(fd) || feof(fd))) {
    bzero(buffer, MAXBUFSIZE); 
    int read = fread(buffer, sizeof(char), MAXBUFSIZE, fd);
    send(sock, buffer,read,0);     
            }
    fclose(fd);
    printf("Finish sending file %s...\n",filename);
    }
    else{
        bzero(buffer,MAXBUFSIZE);
        sprintf(buffer,"File is not available");
        send(sock,buffer,sizeof(buffer),0);
    }
}
void receive_file(char* filename,int sock){
    char dir[MAXBUFSIZE],buffer[MAXBUFSIZE];
    bzero(dir,MAXBUFSIZE);
    sprintf(dir,"./%s/download_files/",c_name);
    strcat(dir,filename);
    FILE* fd = fopen(dir, "ab");
    int numbytes;

    if (fd == NULL) {
     printf("Can't open file\n");           
    } 
    else {
    // receive and write file to local directory
        printf("Start downloading file %s.\n",filename);
        do {
                bzero(buffer, MAXBUFSIZE);
                numbytes=recv(sock, buffer, MAXBUFSIZE, 0);
                if(!strcmp(buffer,"File is not available"))
                {
                    printf("Client: File is not abailable\n");
                    fclose(fd);
                    return;
                }
                if (strlen(buffer) > MAXBUFSIZE) {
                    buffer[strlen(buffer) - 1] = '\0';
                }
                else {
                    fwrite(buffer,sizeof(char),numbytes,fd);
                }
        }  while (strcmp(buffer, ""));
        printf("Finish downloading files.\n");
        fclose(fd);
        }
}
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

#include <netdb.h>
#include <sys/param.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#define MAXBUFSIZE 1024

char* c_name;
char* server_ip;
int server_port;
char* file_list;

char* filelist(char* name, char* file_list,char* buf);

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
 	
	int sock;
	struct hostent *host;
    struct sockaddr_in server_addr;

    host = gethostbyname(server_ip);

    if ((sock = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
        perror("Socket error");
        exit(1);
    }

    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(server_port);
    server_addr.sin_addr = *((struct in_addr*)host->h_addr);
    bzero(&(server_addr.sin_zero), 8);

    if (connect(sock, (struct sockaddr*)&server_addr,
                sizeof(struct sockaddr)) == -1) {
        perror("Connect");
        exit(1);
    }

    //2.send client name and file list to server 
    char buffer[MAXBUFSIZE];
    bzero(buffer,MAXBUFSIZE);
    filelist(c_name,file_list,buffer);
    printf("client %s is sending file_list\n",c_name);
    send(sock,buffer,sizeof(buffer),0);
    bzero(buffer,MAXBUFSIZE);
    recv(sock,buffer,MAXBUFSIZE,0);
    printf("%s\n",buffer);

    char buf[MAXBUFSIZE];
    char recvbuf[MAXBUFSIZE];
    while (1) {
        /* Read command line and store in buffer*/
        bzero(buf,MAXBUFSIZE);
        bzero(buffer,MAXBUFSIZE);
        printf("> ");
        fgets(buf, MAXBUFSIZE, stdin);
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
    	filelist(c_name,file_list,buffer);
    	send(sock,buffer,sizeof(buffer),0);
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
		if(!strcmp(buf,"Server receive file_list successfully")){
			printf("%s\n",recvbuf);
		}
		else if(!strcmp(buf,"filelist")){
			printf("Received list of files from server\nFile name || File size KB || File owner\n%s",recvbuf+fdot+1);
		}
		else if(!strcmp(buf,"ip")){
			printf("ip=%s\n",recvbuf+fdot+1);
		}
    }
close(sock);
}

char* filelist(char* name, char* file_list, char* buf)
{
	char dir[MAXBUFSIZE];
	int size,fdot,sdot;
	char filename[MAXBUFSIZE],hostname[MAXBUFSIZE],buffer[MAXBUFSIZE];
	sdot = 0; fdot=0;
	FILE * f =NULL;
	bzero(buffer,MAXBUFSIZE);
	sprintf(buf,"%s.SendMyFilesList.",name);
	//get ip adress
	bzero(hostname,MAXBUFSIZE);
	gethostname(hostname,MAXBUFSIZE);
	hostname[MAXBUFSIZE-1] = '\0';
	struct hostent * record=gethostbyname(hostname);
	struct in_addr ** addr_list= (struct in_addr **)record->h_addr_list;
	char* ip_address =inet_ntoa(*addr_list[0]);
	
    //copy information to buffer
    while(sdot<strlen(file_list)){
    	bzero(filename,MAXBUFSIZE);
    	bzero(buffer,MAXBUFSIZE);
    	fdot = strcspn(file_list+sdot, ",");
		strncpy(filename, file_list+sdot, fdot);
		filename[fdot] = '\0';
		bzero(dir,MAXBUFSIZE);
		strcpy(dir,"./shared_files/");	
		strcat(dir,filename);
		f=fopen(dir,"r");
		if (f !=NULL)
    	{
    		fseek(f, 0, SEEK_END); // seek to end of file
			size = ftell(f); // get current file pointer
			fseek(f, 0, SEEK_SET); // seek back to beginning of file
    		fclose(f);
    		sprintf(buffer,"%s||%d||%s||%s||",filename,size,name,ip_address);
    	}
		sdot = sdot+fdot +1;
		strcat(buf,buffer);
	}

}
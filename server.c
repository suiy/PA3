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
#define MAX_CONNECTS 50

int server_port;
int hash_size;

typedef struct _node
{
    char *key;
    char *size;
    char *owner;
    char *ip;
    struct _node *next;
}node;
node* hashtable[4];

int tcp_listen(int port);
void hash_ini();
void hash_update(char* buffer, char *owner);
node* read_buf(char* buffer);
node* hash_search(char* key);
void hash_print(char* buffer);
void hash_delete(char* owner);

int main(int argc, char *argv[])
{
	//Read Command Line and set values
    if (argc != 2) {
        printf("usage: %s <server port #>\n", argv[0]);
        exit(1);
    }
    server_port = atoi(argv[1]);
    
    int sock;
    int clientAddrSize = sizeof(struct sockaddr_in);
    struct sockaddr_in clientAddr;
    int servSock;
    char buffer[MAXBUFSIZE];
    char command[MAXBUFSIZE];
    char name[MAXBUFSIZE];


    servSock=tcp_listen(server_port);
    sock = accept(servSock,
            (struct sockaddr*) &clientAddr,
            (socklen_t*) &clientAddrSize);

   hash_ini();
    while(1){
    bzero(buffer,MAXBUFSIZE);
    bzero(command,MAXBUFSIZE);
    bzero(name,MAXBUFSIZE);
    recv(sock,buffer,MAXBUFSIZE,0);
    
    //printf("receive from client\n%s\n",buffer);
    int fdot = strcspn(buffer, ".");
    int sdot = strcspn(buffer+fdot+1,".");
    strncpy(name,buffer,fdot);
    strncpy(command, buffer+fdot+1, sdot);

    if(!strcmp(command,"SendMyFilesList")){
    hash_update(buffer+fdot+sdot+2,name);
    bzero(buffer,MAXBUFSIZE);
    hash_print(buffer);
   printf("%s just started connecting. This is the updated file list from all clients:\nFile name || File size KB || File owner\n%s\n",name,buffer);
    bzero(buffer,MAXBUFSIZE);
    strcpy(buffer,"Server receive file_list successfully.");
    send(sock,buffer,sizeof(buffer),0);
    }
    else if(!strcmp(command,"List")){
    bzero(buffer,MAXBUFSIZE);
    strcpy(buffer,"filelist.");
    hash_print(buffer);
    send(sock,buffer,sizeof(buffer),0);
    }
    else if(!strcmp(command,"Exit")){
        hash_delete(name);
        printf("%s Exit\n",name);
    }
    else{
        node* entry = hash_search(buffer+fdot+sdot+2);
        bzero(buffer,MAXBUFSIZE);
        strcpy(buffer,"ip.");
        if(entry!=NULL){
        strcat(buffer,entry->ip);
        }
        else{
            strcat(buffer,"File requested is not available");
        }
        send(sock,buffer,sizeof(buffer),0);
    }
    }
    close(sock);
    close(servSock);
}

int tcp_listen(int port)
{
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
void hash_ini(node *head){
    int s;
    for(s=0;s<4;s++){
        hashtable[s]=NULL;
    }
}

void hash_update(char* buffer, char *owner){
    node* entry;
    node* next;
    entry=NULL;
    int n,k;
    
    for(n=0;n<4;n++){
        if(hashtable[n]!=NULL){
            if(!strcmp(hashtable[n]->owner,owner)){
                entry = hashtable[n];
                hashtable[n]=NULL;
                while(entry!=NULL){
                next = entry->next;
                free(entry->key);
                free(entry->size);
                free(entry->owner);
                free(entry->ip);
                entry->next=NULL;
                free(entry);
                entry = next;
                }
                hashtable[n]=read_buf(buffer);
                entry = hashtable[n];
                break;
            }
        }
        else{
            k=n;
        }
    }

    if(entry == NULL){
     hashtable[k]=read_buf(buffer);
    }
    
}
node* read_buf(char* buffer){
    char buf[MAXBUFSIZE];
    int first = 1;
    int sdot = 0;
    int fdot;
    node* entry = (node*)malloc(sizeof(node));
    entry->next = NULL;
    node* head = entry;
    while(sdot<strlen(buffer)){ 
        if(first != 1){
            entry = (node*)malloc(sizeof(node));
            entry->next = head;
            head = entry;
        }
        fdot = strcspn(buffer+sdot, "||");
        bzero(buf,MAXBUFSIZE);
        strncpy(buf, buffer+sdot, fdot);
        buf[fdot]='\0';
        entry->key=(char*)malloc(fdot*sizeof(char));
        strcpy(entry->key,buf);
        sdot = sdot+fdot+2;

        fdot = strcspn(buffer+sdot,"||");
        bzero(buf,MAXBUFSIZE);
        strncpy(buf, buffer+sdot, fdot);
         buf[fdot]='\0';
        entry->size=(char*)malloc(fdot*sizeof(char));
        strcpy(entry->size,buf);
        sdot = sdot+fdot+2;
        
        fdot = strcspn(buffer+sdot,"||");
        bzero(buf,MAXBUFSIZE);
        strncpy(buf, buffer+sdot, fdot);
         buf[fdot]='\0';
        entry->owner=(char*)malloc(fdot*sizeof(char));
        strcpy(entry->owner,buf);
        sdot = sdot+fdot+2;
        
        fdot = strcspn(buffer+sdot,"||");
        bzero(buf,MAXBUFSIZE);
        strncpy(buf, buffer+sdot, fdot);
         buf[fdot]='\0';
        entry->ip=(char*)malloc(fdot*sizeof(char));
        strcpy(entry->ip,buf);
        sdot = sdot+fdot+2;
        first = 0;
    }
    return head;
}
node* hash_search(char* key){
int n;
node* entry;
 for(n=0;n<4;n++){
    entry = hashtable[n];
    while(entry!=NULL){
        if(!strcmp(entry->key,key)){
         return entry;
        }
        else{
            entry = entry->next;
        }
    }
}
    return NULL;
}
void hash_print(char* buffer){
    node* entry;
    char buf[MAXBUFSIZE];
    int n;
    for(n=0;n<4;n++){
        if(hashtable[n]!=NULL){
            entry = hashtable[n];
            while(entry!=NULL){
                bzero(buf,MAXBUFSIZE);
            sprintf(buf,"%s||%s||%s\n",entry->key,entry->size,entry->owner);
            strcat(buffer,buf);
            entry = entry->next;
           }
        }
    }
}
void hash_delete(char* owner){
    int n;
     node* entry;
    node* next;
    entry=NULL;

    for(n=0;n<4;n++){
        if(hashtable[n]!=NULL){
            if(!strcmp(hashtable[n]->owner,owner)){
                entry = hashtable[n];
                hashtable[n]=NULL;
                while(entry!=NULL){
                next = entry->next;
                free(entry->key);
                free(entry->size);
                free(entry->owner);
                free(entry->ip);
                entry->next=NULL;
                free(entry);
                entry = next;
                }
                break;
            }
        }
    }
}
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
#include <sys/param.h>
#include <pthread.h>
#include <sys/time.h>

#define MAXBUFSIZE 1024
#define MAX_CONNECTS 50

int server_port;
pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;

typedef struct _node
{
    char *key;
    char *size;
    char *owner;
    char *ip;
    struct _node *next;
}node;

node* hashtable[MAX_CONNECTS];
int socks[MAX_CONNECTS];

int tcp_listen(int port);
void hash_ini();
void hash_update(char* buffer, char *owner,int sock,char *ip);
node* read_buf(char* buffer,char* ip);
node* hash_search(char* key);
void hash_print(char* buffer);
void server_print(char* buffer);
void hash_delete(char* owner);
void *connection(void *);

int main(int argc, char *argv[]){
	//Read Command Line and set values
    if (argc != 2) {
        printf("usage: %s <server port #>\n", argv[0]);
        exit(1);
    }
    server_port = atoi(argv[1]);
    
    int sock=0;
    int clientAddrSize = sizeof(struct sockaddr_in);
    struct sockaddr_in clientAddr;
    int servSock;
    struct timeval currTime;
    pthread_t th;
    int r;
    char buffer[MAXBUFSIZE];

    gettimeofday(&currTime,NULL);

    servSock=tcp_listen(server_port);
    hash_ini();
    while(1){
     // check to see if anyone is trying to connect 
        fd_set rfds;
        struct timeval tv;
        FD_ZERO(&rfds);
        FD_SET(servSock, &rfds);
        tv.tv_sec = 0;
        tv.tv_usec = 100000;
        if (select(servSock + 1, &rfds, NULL, NULL, &tv)) {
            /* if someone is trying to connect, accept() the connection */
            sock = accept(servSock,
                (struct sockaddr*) &clientAddr,
                (socklen_t*) &clientAddrSize);
    //if you've accepted the connection, you'll probably want to
    //check "select()" to see if they're trying to send data, 
        //like their name, and if so
    //recv() whatever they're trying to send
        FD_ZERO(&rfds);
        FD_SET(sock, &rfds);
        tv.tv_sec = 0;
        tv.tv_usec = 100000;
        if (select(sock + 1, &rfds, NULL, NULL, &tv)) {
            
            bzero(buffer,MAXBUFSIZE);
            recv(sock,buffer,MAXBUFSIZE,0);
        char name[MAXBUFSIZE];
        bzero(name,MAXBUFSIZE);
        int fdot = strcspn(buffer,".");
        int sdot = strcspn(buffer+fdot+1,".");
        strncpy(name,buffer,fdot);  
        //since you're talking nicely now.. probably a good idea send them
        //a message to welcome them to the new client.
        char welcome[MAXBUFSIZE];
        bzero(welcome,MAXBUFSIZE);
        sprintf(welcome,"%s, Welcome to the server-client chat room\n.",name);
        send(sock,welcome,sizeof(welcome),0);
        //if there are others connected to the server, probably good to notify them
        //that someone else has joined.
        char ip[MAXBUFSIZE];
        bzero(ip,MAXBUFSIZE);
        strcpy(ip,inet_ntoa(clientAddr.sin_addr));

        pthread_mutex_lock(&mutex);
        //now add your new user to your global list of users
        hash_update(buffer+fdot+sdot+2,name,sock,ip);
        pthread_mutex_unlock(&mutex);
        bzero(buffer,MAXBUFSIZE);
        server_print(buffer);
        printf("%s just started connecting. This is the updated file list from all clients:\nFile name || File size KB || File owner\n%s\n",name,buffer);
        //now you need to start a thread to take care of the 
    //rest of the messages for that client
    
}
r = pthread_create(&th, 0, connection, (void *)sock);
    if (r != 0) { fprintf(stderr, "thread create failed\n"); }
    }
}
    close(sock);
    close(servSock);
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
void *connection(void *sockid) {
    int s = (int)sockid;

    char buffer[MAXBUFSIZE];
    char command[MAXBUFSIZE];
    char name[MAXBUFSIZE];
    char ip[MAXBUFSIZE];
    bzero(ip,MAXBUFSIZE);
    
    pthread_detach(pthread_self());  //automatically clears the threads memory on exit
    
    int n;
    for(n=0;n<MAX_CONNECTS;n++){
    if(s==socks[n]){
        if(hashtable[n]!=NULL){
         strcpy(ip,(hashtable[n]->ip)+5);
         break;
       }
     }
    }


    /*
     * Here we handle all of the incoming text from a particular client.
     */
    while (1)
    {

    bzero(command,MAXBUFSIZE);
    bzero(name,MAXBUFSIZE);
    bzero(buffer,MAXBUFSIZE);
  
    
    recv(s,buffer,MAXBUFSIZE,0);
    
    int fdot = strcspn(buffer, ".");
    int sdot = strcspn(buffer+fdot+1,".");
    strncpy(name,buffer,fdot);
    strncpy(command, buffer+fdot+1, sdot);

    if(!strcmp(command,"SendMyFilesList")){
        pthread_mutex_lock(&mutex);
        hash_update(buffer+fdot+sdot+2,name,s,ip);
        pthread_mutex_unlock(&mutex);
        bzero(buffer,MAXBUFSIZE);
        server_print(buffer);
        printf("%s updated file list:\nFile name || File size KB || File owner\n%s\n",name,buffer);
        bzero(buffer,MAXBUFSIZE);
        strcpy(buffer,"Server: receive file_list successfully.");
        send(s,buffer,sizeof(buffer),0);
    }
    else if(!strcmp(command,"List")){
    bzero(buffer,MAXBUFSIZE);
    strcpy(buffer,"filelist.");
    hash_print(buffer);
    send(s,buffer,sizeof(buffer),0);
    }
    else if(!strcmp(command,"Exit")){
        pthread_mutex_lock(&mutex);
        //remove myself from the vector of active clients
        hash_delete(name);
        pthread_mutex_unlock(&mutex);
        printf("%s Exit\n",name);
        pthread_exit(NULL);
        printf("Shouldn't see this!\n");
    }
    else{
        node* entry = hash_search(buffer+fdot+sdot+2);
        char filename[MAXBUFSIZE];
        bzero(filename,MAXBUFSIZE);
        strcpy(filename,buffer+fdot+sdot+2);
        bzero(buffer,MAXBUFSIZE);
        sprintf(buffer,"ip.%s|",filename);
        if(entry!=NULL){
        strcat(buffer,entry->ip);
        }
        else{
            strcat(buffer,"File requested is not available");
        }
        send(s,buffer,sizeof(buffer),0);
    }
    
    
    //if I received an 'exit' message from this client
    
    
    
    
    //A requirement for 5273 students:
    //if I received a new files list from this client, the
    //server must “Push”/send the new updated hash table to all clients it is connected to.
    
    
    //loop through global client list and
    
    }

    //should probably never get here
    return 0;
}

void hash_ini(node *head){
    int s;
    for(s=0;s<MAX_CONNECTS;s++){
        hashtable[s]=NULL;
        socks[s]=-1;
    }
}

void hash_update(char* buffer, char *owner,int sock,char *ip){
    node* entry;
    node* next;
    entry=NULL;
    int n,k;
    
    for(n=0;n<MAX_CONNECTS;n++){
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
                hashtable[n]=read_buf(buffer,ip);
                entry = hashtable[n];
                break;
            }
        }
        else{
            k=n;
        }
    }

    if(entry == NULL){
     hashtable[k]=read_buf(buffer,ip);
     socks[k]=sock;
    }
    
}
node* read_buf(char* buffer,char* ip){
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
        entry->ip=(char*)malloc((fdot+15)*sizeof(char));
        strcpy(entry->ip,buf);
        strcat(entry->ip,ip);
        sdot = sdot+fdot+2;
        first = 0;
    }
    return head;
}

node* hash_search(char* key){
    int n;
node* entry;
 for(n=0;n<MAX_CONNECTS;n++){
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
    for(n=0;n<MAX_CONNECTS;n++){
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
void server_print(char* buffer){
    node* entry;
    char buf[MAXBUFSIZE];
    int n;
    for(n=0;n<MAX_CONNECTS;n++){
        if(hashtable[n]!=NULL){
            entry = hashtable[n];
            while(entry!=NULL){
            sprintf(buf,"%s||%s||%s||%s\n",entry->key,entry->size,entry->owner,(entry->ip)+5);
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

    for(n=0;n<MAX_CONNECTS;n++){
        if(hashtable[n]!=NULL){
            if(!strcmp(hashtable[n]->owner,owner)){
                entry = hashtable[n];
                hashtable[n]=NULL;
                socks[n]=-1;
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
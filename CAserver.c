/* OpenSSL headers */
#include <openssl/bio.h>
#include <openssl/ssl.h>
#include <openssl/err.h>

#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/time.h>

#define MAX_CONNECTS 50

#define MAXBUFSIZE 1024
#define ON   1
#define OFF        0




void* connection(void *);
char* trim(char*);
int tcp_listen(int);
SSL_CTX* initialize_ctx(char*, char*, char*);
void receive_file(char*,SSL*);


pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;

int server_port;
FILE* log_file;
char* ca_cert;
char* server_cert;
char* server_key;

BIO* bio_err;

int main(int argc, char* argv[])
{
    pthread_t th;

    int connected;

    struct sockaddr_in client_addr;
    unsigned int sin_size;

    if (argc != 2)  {
	printf("usage: %s <port>\n", argv[0]);
	return 0;
    }


    SSL_library_init();
    OpenSSL_add_all_algorithms();
    SSL_load_error_strings();
    bio_err = BIO_new_fp(stderr, BIO_NOCLOSE);
                              
    server_port = atoi(argv[1]);
    log_file = fopen(argv[2], "w");
    ca_cert = "./pki_jungle/myCA/myca.crt";
    server_cert = "./ca.crt";
    server_key = "./privket-init.key";
    X509            *client_cert = NULL;
    char    *str;

    /* 1. Create socket/listen
     */
    int sock = tcp_listen(server_port);

    /* 2. Configure SSL/TLS connection parameters (such as encryption/hashing
     *    algorithms, paths to keys, etc)
     */
    SSL_CTX* ctx = initialize_ctx(server_cert, server_key, ca_cert);

    while (1) {
        /* check to see if anyone is trying to connect */
        fd_set rfds;
        struct timeval tv;
        FD_ZERO(&rfds);
        FD_SET(sock, &rfds);
        tv.tv_sec = 0;
        tv.tv_usec = 100000;
        if (select(sock + 1, &rfds, NULL, NULL, &tv)) {
            /* if someone is trying to connect, accept() the connection */
            sin_size = sizeof(struct sockaddr_in);
            connected = accept(sock, (struct sockaddr*)&client_addr, &sin_size);
            
            /* 3. Create SSL connection (the libraries handle the below steps in
             *    the connection functions --- these are just for information
             *    purposes)
             *      1. Send server's public key; receive server's public key
             *      2. Verify client's public key signed by trusted CA
             *      3. Symmetric Key Exchange
             */
            BIO* sbio = BIO_new_socket(connected, BIO_NOCLOSE);
            SSL* ssl = SSL_new(ctx);
            SSL_set_bio(ssl, sbio, sbio);
            if (SSL_accept(ssl) <= 0) {
                printf("SSL_accept\n");
                ERR_print_errors(bio_err);
                exit(1);
            }
 
          /* Get the client's certificate (optional) */
       client_cert = SSL_get_peer_certificate(ssl);
        if (client_cert != NULL) 
           {
             printf ("Client certificate:\n");     
              str = X509_NAME_oneline(X509_get_subject_name(client_cert), 0, 0);
                   printf ("\t subject: %s\n", str);
                   free (str);
                 str = X509_NAME_oneline(X509_get_issuer_name(client_cert), 0, 0);
                   printf ("\t issuer: %s\n", str);
                    free (str);
                 X509_free(client_cert);
     } 
 
     else
 
                   printf("The SSL client does not have certificate.\n");
  
 

            /* start a new thread to take care of this new user */
         int   r = pthread_create(&th, 0, connection, (void *)ssl);
    if (r != 0) { fprintf(stderr, "thread create failed\n"); }
            
        }
    }

    close(sock);
    
}
void receive_file(char* filename,SSL *ssl){
    char dir[MAXBUFSIZE],buffer[MAXBUFSIZE];
    bzero(dir,MAXBUFSIZE);
    sprintf(dir,"./pki_jungle/");
    strcat(dir,filename);
    FILE* fd = fopen(dir, "ab");
    int numbytes;

    if (fd == NULL) {
     printf("Can't open file\n");           
    } 
    else {
    // receive and write file to local directory
        printf("Start downloading csr %s.\n",filename);
        do {
                bzero(buffer, MAXBUFSIZE);
                numbytes=SSL_read(ssl, buffer, MAXBUFSIZE);
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
void* connection(void* ssl) {
    SSL* ssl_id = (SSL*) ssl;


    pthread_detach(pthread_self());
receive_file("client1.csr",ssl_id);

    return 0;
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

SSL_CTX* initialize_ctx(char* server_cert, char* server_key, char* ca_cert)
{
    SSL_CTX* ctx = SSL_CTX_new(SSLv23_server_method());
    if (!ctx) {
 
            ERR_print_errors_fp(stderr);
 
           exit(1);
 
       }

    if (!SSL_CTX_use_certificate_chain_file(ctx, server_cert)) {
        printf("SSL_CTX_use_certificate_file\n");
        exit(1);
    }
    if (!SSL_CTX_use_PrivateKey_file(ctx, server_key, SSL_FILETYPE_PEM)) {
        printf("SSL_CTX_use_PrivateKey_file\n");
        exit(1);
    }
    if (!SSL_CTX_load_verify_locations(ctx, ca_cert, 0)) {
        printf("SSL_CTX_load_verify_locations\n");
        exit(1);
    }
    
    if (!SSL_CTX_check_private_key(ctx)) {
 
                 fprintf(stderr,"Private key does not match the certificate public key\n");
                  exit(1);
    }
 
              /* Load the RSA CA certificate into the SSL_CTX structure */
                if (!SSL_CTX_load_verify_locations(ctx, ca_cert, NULL)) {
 
                   ERR_print_errors_fp(stderr);
                        exit(1);
            }
 
              /* Set to require peer (client) certificate verification */
         SSL_CTX_set_verify(ctx,SSL_VERIFY_PEER,NULL);
 
          /* Set the verification depth to 1 */
               SSL_CTX_set_verify_depth(ctx,1);
 
    /*
    if (!SSL_CTX_set_cipher_list(ctx, "DHE-RSA-AES256-SHA:DHE-DSS-AES256-SHA:AES256-SHA:EDH-RSA-DES-CBC3-SHA:EDH-DSS-DES-CBC3-SHA:DES-CBC3-SHA:DES-CBC3-MD5:DHE-RSA-AES128-SHA:DHE-DSS-AES128-SHA:AES128-SHA:RC2-CBC-MD5:RC4-SHA:RC4-MD5:RC4-MD5:EDH-RSA-DES-CBC-SHA:EDH-DSS-DES-CBC-SHA:DES-CBC-SHA:DES-CBC-MD5:EXP-EDH-RSA-DES-CBC-SHA:EXP-EDH-DSS-DES-CBC-SHA:EXP-DES-CBC-SHA:EXP-RC2-CBC-MD5:EXP-RC2-CBC-MD5:EXP-RC4-MD5:EXP-RC4-MD5")) {
        printf("SSL_CTX_set_cipher_list\n");
        exit(1);
    }
    SSL_CTX_set_verify(ctx, SSL_VERIFY_PEER|SSL_VERIFY_FAIL_IF_NO_PEER_CERT, 0);
*/
    return ctx;
}


char* trim(char* str)
{
    if (strlen(str) > 0 && str[strlen(str) - 1] == '\n') {
        str[strlen(str) - 1] = 0;
    }
    return str;
}
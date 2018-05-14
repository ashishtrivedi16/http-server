//
//  server.c
//  Web-server-v2
//
//  Created by Ashish Trivedi on 13/05/18.
//  Copyright © 2018 Ashish Trivedi. All rights reserved.
//

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>
#include <netinet/in.h>
#include <pthread.h>
#include <ctype.h>
#include <sys/stat.h>
#include <errno.h>

#define SERVER_STRING "Server: athttp-server/2.0\r\n"

void *acceptRequest(void * client);
void errorMessage(const char* msg);
int get_line(int sock, char *buf, int size);
int startup(u_short port);
void serve_file(int client, const char *filename);
void not_found(int client);
void headers(int client, const char *filename);
void cat(int client, FILE *resource);

/**********************************************************************/
/* Get a line from a socket, whether the line ends in a newline,
 * carriage return, or a CRLF combination.  Terminates the string read
 * with a null character.  If no newline indicator is found before the
 * end of the buffer, the string is terminated with a null.  If any of
 * the above three line terminators is read, the last character of the
 * string will be a linefeed and the string will be terminated with a
 * null character.
 */
/**********************************************************************/
int get_line(int sock, char *buff, int size){
    int i = 0;
    char c = '\0';
    long n;
    
    while ((i < size - 1) && (c != '\n'))
    {
        n = recv(sock, &c, 1, 0);
        /* DEBUG printf("%02X\n", c); */
        if (n > 0)
        {
            if (c == '\r')
            {
                n = recv(sock, &c, 1, MSG_PEEK);
                /* DEBUG printf("%02X\n", c); */
                if ((n > 0) && (c == '\n'))
                    recv(sock, &c, 1, 0);
                else
                    c = '\n';
            }
            buff[i] = c;
            i++;
        }
        else
            c = '\n';
    }
    buff[i] = '\0';
    
    return i;
}

/**********************************************************************/
/* A request has caused a call to accept() on the server port to
 * return.
 * This function is executed by a seperate thread which is created when
 * a connection is received */
/**********************************************************************/
void *acceptRequest(void * client) {
    char buff[1024];
    int numchars;
    int method[255];
    char path[512];
    size_t i, j;
    int client_socket = (int)client;
    numchars = get_line(client_socket, buff, sizeof(buff));
    /* Find out where everything is */
    
    i = 0; j = 0;
    while (!isspace((int)buff[j]) && (i < sizeof(method) - 1))
    {
        method[i] = buff[j];
        i++; j++;
    }
    method[i] = '\0';
    
    char *url  = NULL;
    
    if(strtok(buff, " "))
    {
        url = strtok(NULL, " ");
        if(url)
            url = strdup(path);
    }
    
    sprintf(path, "htdocs/%s", url);
    if (path[strlen(path) - 1] == '/')
        strcat(path, "index.html");
    
    serve_file(client_socket, path); // serves file
    close(client_socket);
    return 0;
}

/**********************************************************************/
/* Send a regular file to the client.  Use headers, and report
 * errors to client if they occur.
 */
/**********************************************************************/
void serve_file(int client, const char *filename) {
    FILE *resource = NULL;
    resource = fopen(filename, "r");
    if (resource == NULL){
        printf("%s\n", strerror(errno));
        not_found(client);
    } else {
        headers(client, filename);
        cat(client, resource);
    }
    fclose(resource);
}

/**********************************************************************/
/* Give a client a 404 not found status message. */
/**********************************************************************/
void not_found(int client) {
    char buf[1024];
    
    sprintf(buf, "HTTP/1.0 404 NOT FOUND\r\n");
    send(client, buf, strlen(buf), 0);
    sprintf(buf, SERVER_STRING);
    send(client, buf, strlen(buf), 0);
    sprintf(buf, "Content-Type: text/html\r\n");
    send(client, buf, strlen(buf), 0);
    sprintf(buf, "\r\n");
    send(client, buf, strlen(buf), 0);
    sprintf(buf, "<HTML><TITLE>Not Found</TITLE>\r\n");
    send(client, buf, strlen(buf), 0);
    sprintf(buf, "<BODY><P>The server could not fulfill\r\n");
    send(client, buf, strlen(buf), 0);
    sprintf(buf, "your request because the resource specified\r\n");
    send(client, buf, strlen(buf), 0);
    sprintf(buf, "is unavailable or nonexistent.\r\n");
    send(client, buf, strlen(buf), 0);
    sprintf(buf, "</BODY></HTML>\r\n");
    send(client, buf, strlen(buf), 0);
}

/**********************************************************************/
/* Return the informational HTTP headers about a file. */
/**********************************************************************/
void headers(int client, const char *filename) {
    char buf[1024];
    (void)filename;  /* could use filename to determine file type */
    
    strcpy(buf, "HTTP/1.0 200 OK\r\n");
    send(client, buf, strlen(buf), 0);
    strcpy(buf, SERVER_STRING);
    send(client, buf, strlen(buf), 0);
    sprintf(buf, "Content-Type: text/html\r\n");
    send(client, buf, strlen(buf), 0);
    strcpy(buf, "\r\n");
    send(client, buf, strlen(buf), 0);
}

/**********************************************************************/
/* Put the entire contents of a file out on a socket */
/**********************************************************************/
void cat(int client, FILE *resource) {
    char buf[1024];
    fgets(buf, sizeof(buf), resource);
    while (!feof(resource))
    {
        send(client, buf, strlen(buf), 0);
        fgets(buf, sizeof(buf), resource);
    }
}
/********************************************************************/
/*
 * Creates socket and starts listening on given port
 * bzero function zeroes various feild in server_addr struct
 */
/********************************************************************/
int startup(u_short port){
    /* Socket creating */
    int serverfd = socket(AF_INET, SOCK_STREAM, 0);
    if (serverfd < 0) {
        fprintf(stderr, "Error opening socket");
        return 0;
    }
    struct sockaddr_in server_addr;
    
    bzero((char*)&server_addr, sizeof(server_addr));
    
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(9001);
    server_addr.sin_addr.s_addr = INADDR_ANY;
    
    if (bind(serverfd, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        errorMessage("Error in connection");
        return 0;
    }
    
    if(listen(serverfd, 5) < 0){
        errorMessage("Error listening");
        return 0;
    }
    printf("Server listening at port 9001...\n");
    return serverfd;
}

/**********************************************************************/
/* Just a helping functions for error reporting using perror
 */
/**********************************************************************/
void errorMessage(const char* msg){
    perror(msg);
}

/********************************************************************/
/********************************************************************/
int main(){
    int server_socket = -1;
    u_short port = 9001;
    pthread_t newthread;
    server_socket = startup(port);
    
    struct sockaddr client_addr;
    long clientfd;
    int client_len = 0;
    while (1) {
        client_len = sizeof(client_addr);
        clientfd = accept(server_socket, (struct sockaddr*)&client_addr, (socklen_t*)&client_len);
        printf("Connection received\n");
        if (clientfd < 0) {
            errorMessage("Error accepting connection");
        }
        if (pthread_create(&newthread , NULL, acceptRequest, (void *)clientfd) != 0)
            errorMessage("pthread creation");
        }
    return 0;
}

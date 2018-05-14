#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>
#include <netinet/in.h> 
#include <pthread.h> // Used for creating threads
#include <ctype.h> // Used for isspace() function
#include <errno.h> // error reporting using stderror(errno)

#define SERVER_STRING "Server: athttp-server/2.0\r\n"

int startup(u_short port);
void not_found(int client);
void content(int client, FILE *resource);
void *acceptRequest(void * client);
void errorMessage(const char* msg);
int get_line(int sock, char *buf, int size);
void serve_file(int client, const char *filename);
void headers(int client, const char *filename);

/**********************************************************************/
/* Just a helping functions for error reporting using perror */
/**********************************************************************/
void errorMessage(const char* msg){
    perror(msg);
}

/**********************************************************************/
/* Send a regular file to the client.  Use headers, and report
 * errors to client if they occur. */
/**********************************************************************/
void serve_file(int client, const char *filename) {
    FILE *resource = NULL;
    resource = fopen(filename, "r");
    if (resource == NULL){
        printf("%s\n", strerror(errno));
        not_found(client);
    } else {
        headers(client, filename);
        content(client, resource);
    }
    fclose(resource);
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
void content(int client, FILE *resource) {
    char buf[1024];
    fgets(buf, sizeof(buf), resource);
    while (!feof(resource))
    {
        send(client, buf, strlen(buf), 0);
        fgets(buf, sizeof(buf), resource);
    }
}

/**********************************************************************/
/* Give a client a 404 not found 
us message. */
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
/* A request causes a call to accept() on the server port.
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
	
    /* This section gets the methods used */
    i = 0; j = 0;
    while (!isspace((int)buff[j]) && (i < sizeof(method) - 1))
    {
        method[i] = buff[j];
        i++; j++;
    }
    method[i] = '\0';
    
	/* This section gets the url requested */
    char *url  = NULL;
    if(strtok(buff, " "))
    {
        url = strtok(NULL, " ");
        if(url)
            url = strdup(path);
    }
    
	/* This section identifies the path of file requested */
    sprintf(path, "htdocs/%s", url);
    if (path[strlen(path) - 1] == '/')
        strcat(path, "index.html");
    /* serve_file function serves the requested file */
    serve_file(client_socket, path);
    close(client_socket);
    return 0;
}

/********************************************************************/
/* Creates socket and starts listening on given port
 * bzero function zeroes various feild in server_addr struct 
 * This function basically implements the create socket, bind socket,
 * and listen function. */
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
    
	/* Binding */
    if (bind(serverfd, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        errorMessage("Error in connection");
        return 0;
    }
    
	/* Listening */
    if(listen(serverfd, 5) < 0){
        errorMessage("Error listening");
        return 0;
    }
    printf("Server listening at port 9001...\n");
    return serverfd;
}

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

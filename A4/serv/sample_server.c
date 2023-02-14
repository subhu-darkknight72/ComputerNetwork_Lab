/**httpserver.c**/
#include "stdio.h"
#include "stdlib.h"
#include "sys/types.h"
#include "sys/socket.h"
#include "string.h"
#include "netinet/in.h"
#include "time.h"
#include "dirent.h"
#include "netdb.h"
#include <unistd.h>
#include <arpa/inet.h>

#define MAX_SIZE 1000000
#define BUF_SIZE 1024
#define CLADDR_LEN 100

int createSocket(char *host, int port);
int listenForRequest(int sockfd);
char *getFileType(char *file);
void get_func();
void put_func();
void receiveStr(char *str, int socket_id);
void sendStr(char *str, int socket_id);

int main(int argc, char **argv)
{
    if(strcmp(argv[1],"GET")==0)
        get_func();
    else if(strcmp(argv[1],"PUT")==0)
        put_func();
    else
        printf("Invalid command");
    return 0;
}
void get_func(){
    DIR *dirptr;
    FILE *fileptr;
    time_t timenow;
    struct tm *timeinfo;
    time(&timenow);
    timeinfo = localtime(&timenow);

    char *header, *request, *path, *newpath, *host;
    char *dir, *temp;
    int port, sockfd, connfd;
    char get[3], http[9];
    char filepath[MAX_SIZE];
    char http_not_found[] = "HTTP/1.0 404 Not Found\n";
    char http_ok[] = "HTTP/1.0 200 OK\n";
    char buffer[MAX_SIZE];
    char *contentType;

    // if (argc != 4)
    // {
    //     printf("usage: [host] [directory] [portnumber]\n");
    //     exit(1);
    // }

    header = (char *)malloc(MAX_SIZE * sizeof(char));
    request = (char *)malloc(MAX_SIZE * sizeof(char));
    path = (char *)malloc(MAX_SIZE * sizeof(char));
    newpath = (char *)malloc(MAX_SIZE * sizeof(char));

    // host = argv[1];
    // dir = argv[2];
    // port = atoi(argv[3]);

    host = (char *)calloc(10000, sizeof(char));
    dir = (char *)calloc(10000, sizeof(char));
    strcpy(host, "127.0.0.1");
    strcpy(dir, "/Users/subhu/Desktop/Sem/Sem_6/CN_Lab/ComputerNetworkLab/A4/serv/");
    port = atoi("8080");

    if ((dirptr = opendir(dir)) == NULL)
    {
        printf("Directory Not Found!\n");
        exit(1);
    }

    sockfd = createSocket(host, port);
    while(1)
    {
        printf("--------------------------------------------------------\n");
        printf("Waiting for a connection...\n");
        connfd = listenForRequest(sockfd);
        // gets the request from the connection
        printf("connfd1:%d\n",connfd);
        int temp_con=connfd;
        recv(connfd, request, 100, 0);
        
        printf("$%s$\n",request);
        
        printf("Processing request...\n");
        
        // parses request
        sscanf(request, "%s %s %s", get, path, http);
        //printf("connfd2:%d\n",connfd); // file descriptor is changing after sscanf command
        
        newpath = path + 1; // ignores the first slash
        sprintf(filepath, "%s/%s", dir, newpath);
        
        printf("filepath= $%s$\n",filepath);
        printf("dir= $%s$\n",dir);
        printf("newpath= $%s$\n",newpath);
        
        contentType = getFileType(newpath);
        sprintf(header, "Date: %sHostname: %s:%d\nLocation: %s\nContent-Type: %s\n\n", asctime(timeinfo), host, port, newpath, contentType);
        
        if ((fileptr = fopen(filepath, "r")) == NULL)
        {
            printf("File not found!\n");
            connfd=temp_con;
            send(connfd, http_not_found, strlen(http_not_found), 0); // sends HTTP 404
        }
        else
        {
            printf("Sending the file...\n");

            // http_ok = (char *)calloc(10000, sizeof(char));
            // strcpy(http_ok, "HTTP/1.0 200 OK");
            printf("$%s$\n",http_ok);
            //printf("connfd:%d\n",connfd);  // file descriptor is changing
            connfd=temp_con;
            ssize_t bytes_sent = send(connfd, http_ok, strlen(http_ok)+1, 0); // sends HTTP 200 OK
            printf("bytes_sent: %zd, length of http_ok: %lu\n",bytes_sent, strlen(http_ok));

            memset(buffer, 0, MAX_SIZE);
            recv(connfd, buffer, MAX_SIZE, 0);
            printf("$%s$\n",buffer);
            if ((temp = strstr(buffer, "OK")) == NULL)
            {
                printf("Operation aborted by the user!\n");
                break;
            }
            send(connfd, header, strlen(header), 0); // sends the header
            recv(connfd, buffer, MAX_SIZE, 0);
            if ((temp = strstr(buffer, "OK")) == NULL)
            {
                printf("Operation aborted by the user!\n");
                break;
            }
            memset(&buffer, 0, sizeof(buffer));
            while (!feof(fileptr))
            { // sends the file
                fread(&buffer, sizeof(buffer), 1, fileptr);
                send(connfd, buffer, sizeof(buffer), 0);
                memset(&buffer, 0, sizeof(buffer));
            }
            printf("File sent...\n");
        }
        printf("Processing completed...\n");
        close(connfd);
    }

    close(sockfd);
    free(header);
    free(request);
    free(path);
    free(newpath);
}

void put_func(){
    printf("put_func");
}

int createSocket(char *host, int port)
{
    int sockfd;
    struct sockaddr_in addr;
    struct hostent *host_ent;
    char *hostAddr;

    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port = htons((short)port);

    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0)
    {
        printf("Error creating socket!\n");
        exit(1);
    }
    printf("Socket created...\n");

    if (bind(sockfd, (struct sockaddr *)&addr, sizeof(addr)) < 0)
    {
        printf("Error binding socket to port!\n");
        exit(1);
    }
    printf("Binding done...\n");

    return sockfd;
}

int listenForRequest(int sockfd)
{
    int conn;
    char hostip[32];
    struct sockaddr_in addr;
    struct hostent *host;
    struct in_addr inAddr;
    int len;

    addr.sin_family = AF_INET;

    listen(sockfd, 5); // maximum 5 connections
    len = sizeof(addr);
    if ((conn = accept(sockfd, (struct sockaddr *)&addr, (socklen_t *)&len)) < 0)
    {
        printf("Error accepting connection!\n");
        exit(1);
    }
    printf("Connection accepted...\n");

    inet_ntop(AF_INET, &(addr.sin_addr), hostip, 32);
    inet_pton(AF_INET, hostip, &inAddr);
    host = gethostbyaddr(&inAddr, sizeof(inAddr), AF_INET);

    printf("---Connection received from: %s [IP= %s]---\n", host->h_name, hostip);
    return conn;
}

char *getFileType(char *file)
{
    char *temp;
    if ((temp = strstr(file, ".html")) != NULL)
    {
        return "text/html";
    }
    else if ((temp = strstr(file, ".pdf")) != NULL)
    {
        return "application/pdf";
    }
    else if ((temp = strstr(file, ".txt")) != NULL)
    {
        return "text/html";
    }
    else if (((temp = strstr(file, ".jpeg")) != NULL) || ((temp = strstr(file, ".heic")) != NULL))
    {
        return "image/jpeg";
    }
    return "Error aa gaya!!";
}

void sendStr(char *str, int socket_id)
{
	int pos, i, len = strlen(str);
	char buf[BUF_SIZE];

	for (pos = 0; pos < len; pos += BUF_SIZE)
	{
		for (i = 0; i < BUF_SIZE; i++)
			buf[i] = ((pos + i) < len) ? str[pos + i] : '\0';

		// send(socket_id, buf, BUF_SIZE, 0);
		if (send(socket_id, buf, BUF_SIZE, 0) < 0)
		{
			perror("error in transmission.\n");
			exit(-1);
		}
	}
}

void receiveStr(char *str, int socket_id)
{
	int flag = 0, i, pos = 0;
	char buf[BUF_SIZE];
	while (flag == 0)
	{
		// recv(socket_id, buf, BUF_SIZE, 0)
		if (recv(socket_id, buf, BUF_SIZE, 0) < 0)
		{
			perror("error in transmission.\n");
			exit(-1);
		}
		// printf("$%s$\n",buf);

		for (i = 0; i < BUF_SIZE && flag == 0; i++)
			if (buf[i] == '\0')
				flag = 1;

		for (i = 0; i < BUF_SIZE; i++, pos++)
			str[pos] = buf[i];
	}
}
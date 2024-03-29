/**httpclient.c**/
#include "stdio.h"
#include "stdlib.h"
#include "sys/types.h"
#include "sys/socket.h"
#include "string.h"
#include "netinet/in.h"
#include "dirent.h"
#include "netdb.h"
#include <unistd.h>
#include <arpa/inet.h>

#define MAX_SIZE 1000000
#define BUF_SIZE 1024

int get_request(char *url, char *port);
int isValidIP(char *ip);
int parseHeader(char *header);
char *splitKeyValue(char *line, int index);
void openFile();

FILE *fileptr;
char keys[][25] = {"Date: ", "Hostname: ", "Location: ", "Content-Type: "};
char status[4] = {0, 0, 0, 0};
char contentFileType[100];
char path[1000];

struct sockaddr_in addr, cl_addr;
int sockfd, ret;
struct hostent *server;
char *url, *temp;
int portNumber;
char *fileName;
char status_ok[] = "OK";
char buffer[BUF_SIZE];
char http_not_found[] = "HTTP/1.0 404 Not Found";
char http_ok[] = "HTTP/1.0 200 OK";
char location[] = "Location: ";
char contentType[] = "Content-Type: ";
int sPos, ePos;

int main(int argc, char **argv){
    url = (char *)calloc(10000, sizeof(char));
    // strcpy(url, "127.0.0.1/gg.txt");
    strcpy(url, "127.0.0.1/IMG_5968.heic");
    // strcpy(url, "127.0.0.1/TimeTable_Sem6.pdf");
    char *port_n = "8081";
    portNumber = atoi(port_n);

    // checking the protocol specified
    if(strcmp(argv[0], "GET") != 0){
        if ((temp = strstr(url, "http://")) != NULL)
            url = url + 7;
        else if ((temp = strstr(url, "https://")) != NULL)
            url = url + 8;

        // checking the port number
        if (portNumber > 65536 || portNumber < 0){
            printf("Invalid Port Number!");
            exit(1);
        }

        printf("url: $%s$\n",url);
        sockfd = get_request(url, port_n);

        memset(buffer, 0, sizeof(buffer));
        ret = recv(sockfd, buffer, BUF_SIZE, 0);
        printf("ret: %d buffer1: $%s$\n",ret,buffer);
        if (ret < 0)
        {
            printf("Error receiving HTTP status!\n");
        }
        else
        {
            printf("%s\n", buffer);
            if ((temp = strstr(buffer, http_ok)) != NULL)
            {
                send(sockfd, status_ok, strlen(status_ok), 0);
            }
            else
            {
                close(sockfd);
                return 0;
            }
        }

        memset(&buffer, 0, sizeof(buffer));
        ret = recv(sockfd, buffer, BUF_SIZE, 0);
        if (ret < 0)
        {
            printf("Error receiving HTTP header!\n");
        }
        else
        {
            printf("buffer2: %s\n", buffer);

            if (parseHeader(buffer) == 0)
            {
                send(sockfd, status_ok, strlen(status_ok), 0);
            }
            else
            {
                printf("Error in HTTP header!\n");
                close(sockfd);
                return 0;
            }
        }

        // printf("file: [%s]\n", fileName);
        // strcpy(path, path+1);
        // strcpy(path, "gg.txt");
        int i=1;
        for(; path[i]!='\0'; i++)
            path[i-1]=path[i];
        path[i-1]='\0';

        printf("path: %s\n",path); // where is path getting initialized (in get_request)
        fileptr = fopen(path, "w");
        if (fileptr == NULL)
        {
            printf("Error opening file!\n");
            close(sockfd);
            return 0;
        }
        else
            printf("opened file!\n");

        memset(&buffer, 0, sizeof(buffer));
        while (recv(sockfd, buffer, BUF_SIZE, 0) > 0)
        { // receives the file
            if ((strstr(contentFileType, "text/html")) != NULL)
            {
                fprintf(fileptr, "%s", buffer);
            }
            else
            {
                fwrite(&buffer, sizeof(buffer), 1, fileptr);
            }
            memset(&buffer, 0, sizeof(buffer));
        }
        fclose(fileptr);
        openFile();
    }
    close(sockfd);
    return 0;
}


int get_request(char *url, char *port)
{
    int sockfd, bindfd;
    char *ptr, *host;
    char getrequest[1024];
    struct sockaddr_in addr;

    if (isValidIP(url))
    { // when an IP address is given
        sprintf(getrequest, "GET / HTTP/1.0\nHOST: %s\n\n", url);
    }
    else
    { // when a host name is given
        if ((ptr = strstr(url, "/")) == NULL)
        {
            // when hostname does not contain a slash
            sprintf(getrequest, "GET / HTTP/1.0\nHOST: %s\n\n", url);
        }
        else
        {
            // when hostname contains a slash, it is a path to file
            strcpy(path, ptr);
            host = strtok(url, "/");

            // printf("path: $%s$\n",path);
            // printf("url: $%s$\n",url);
            sprintf(getrequest, "GET %s HTTP/1.0\nHOST: %s\n\n", path, url);
            // printf("get_req: $%s$\n",getrequest);
        }
    }
    // creates a socket to the host
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0)
    {
        printf("Error creating socket!\n");
        exit(1);
    }
    printf("Socket created...\n");

    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = inet_addr(url);
    addr.sin_port = htons(atoi(port));

    if (connect(sockfd, (struct sockaddr *)&addr, sizeof(addr)) < 0)
    {
        printf("Connection Error!\n");
        exit(1);
    }

    printf("Connection successful...\n\n\n");
    //ptr=path+1;
    //strcpy(path,ptr);
    // ptr = strtok(path, "/");
    // ptr= path+1;

    ///printf("path: $%s$\n",path);
    ///printf("ptr: $%s$\n",ptr);
    // char *ptemp = ptr;
    // while(ptemp!=NULL){
    //     printf("$%s$ ",ptemp);
    //     ptemp = strtok(NULL, "/");
    // }
    // printf("\n");

    // printf("\nGG-2-\n");
    // printf("$%s$",ptr);
    // printf("\nGG-3-\n");

    // strcpy(path, ptr);
    // printf("path=%s\n", path);
    // fileptr = fopen(path, "w");
    // strcpy(fileName, path);
    // sprintf(fileName, "%s", path);

    // int optval = 1;
    // setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof optval);

    // writes the HTTP GET Request to the sockfd
    // printf("\nGG-0-\n");
    // printf("$%s$\n",getrequest);
    // write(sockfd, getrequest, strlen(getrequest));
    send(sockfd, getrequest, strlen(getrequest), 0);
    // printf("\nGG-1-\n");
    return sockfd;
}

int isValidIP(char *ip)
{
    struct sockaddr_in addr;
    int valid = inet_pton(AF_INET, ip, &(addr.sin_addr));
    return valid != 0;
}

int parseHeader(char *header)
{
    //"Date: %sHostname: %s:%d\nLocation: %s\nContent-Type: %s\n\n"
    char *line, *key, *value;
    char temp[100];
    int i = 0;
    line = strtok(header, "\n");
    while (line != NULL)
    {
        // printf("%s\n", line);
        strcpy(temp, line);
        value = splitKeyValue(line, i);
        if (i == 3)
        {
            strcpy(contentFileType, value);
        }
        // printf("value=%s\n", value);
        line = strtok(NULL, "\n");
        i++;
    }
    for (i = 0; i < 4; i++)
    {
        if (status[i] == 0)
            return 1;
        // printf("status[%d]=%d\n", i, status[i]);
    }
    return 0;
}

char *splitKeyValue(char *line, int index)
{
    char *temp;
    if ((temp = strstr(line, keys[index])) != NULL)
    {
        temp = temp + strlen(keys[index]);
        status[index] = 1;
    }
    return temp;
}

void openFile()
{
    char *temp;
    char command[100];
    char fileName[1000];
    strcpy(fileName, path);
    // printf("File Name: %s\n", fileName);
    // printf("Content Type: %s\n", contentFileType);
    if ((temp = strstr(contentFileType, "text/html")) != NULL)
    {
        if ((temp = strstr(fileName, ".txt")) != NULL)
        {
            sprintf(command, "open -a TextEdit %s", fileName);
        }
        else
        {
            sprintf(command, "firefox %s", fileName);
        }
        system(command);
    }
    else if ((temp = strstr(contentFileType, "application/pdf")) != NULL)
    {
        sprintf(command, "open -a Preview.app %s", fileName);
        system(command);
    }
    else if ((temp = strstr(contentFileType, "image/jpeg")) != NULL)
    {
        sprintf(command, "open -a Preview.app %s", fileName);
        system(command);
    }
    else
    {
        printf("The filetype %s is not supported. Failed to open %s!\n", contentFileType, fileName);
    }
}
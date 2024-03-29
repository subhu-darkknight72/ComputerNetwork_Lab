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
#include <poll.h>
#include<time.h>

#define MAX_SIZE 1000000
#define BUF_SIZE 1024

int get_request(char *url, char *port);
int put_request(char *url, char *port);
int isValidIP(char *ip);
int parseHeader(char *header);
char *splitKeyValue(char *line, int index);
void openFile();
char *get_filename(char *url_var);
char* get_date();
char *get_date_mod();
char *getFileType(char *file);

void get_func();
void put_func();

char **get_arg(char *cmd);
int runExtCmd(char *usr_cmd);

void receiveStr(char *str, int socket_id);
void sendStr(char *str, int socket_id);

FILE *fileptr;
char keys[][25] = {"Date: ", "Hostname: ", "Location: ", "Content-Type: "};
char status[4] = {0, 0, 0, 0};
char *contentFileType;
char path[1000];
char *file_name;
char *url, *port_n;
char *header, *host;

int main(int argc, char **argv)
{
    file_name = (char *)calloc(1000, sizeof(char));
    url = (char *)calloc(10000, sizeof(char));
    strcpy(url, argv[2]);
    printf("url: $%s$\n",url);

    port_n = (char *)calloc(100, sizeof(char));
    strcpy(port_n, "8080");

    if(argc ==3 && strcmp(argv[1],"GET")==0){
        strcpy(file_name, get_filename(url));
        strcpy(url, argv[2]);
        get_func();
    }
    else if(argc ==4 && strcmp(argv[1],"PUT")==0){
        strcpy(file_name, argv[3]);
        strcpy(url, argv[2]);
        put_func();
    }
    else
        printf("Invalid command");
    return 0;
}

void get_func(){
    struct sockaddr_in addr, cl_addr;
    int sockfd, ret; 
    struct hostent *server;
    char *temp;
    int portNumber;
    
    char status_ok[] = "OK";
    char buffer[MAX_SIZE];
    char http_not_found[] = "HTTP/1.0 404 Not Found";
    char http_ok[] = "HTTP/1.0 200 OK";
    char location[] = "Location: ";
    char contentType[] = "Content-Type: ";
    int sPos, ePos;

    printf("url: $%s$\n",url);
    contentFileType = (char *)calloc(100, sizeof(char));
    contentFileType = getFileType(file_name);

    portNumber = atoi(port_n);

    // checking the protocol specified
    if ((temp = strstr(url, "http://")) != NULL)
    {
        url = url + 7;
    }
    else if ((temp = strstr(url, "https://")) != NULL)
    {
        url = url + 8;
    }

    // checking the port number
    if (portNumber > 65536 || portNumber < 0)
    {
        printf("Invalid Port Number!");
        exit(1);
    }

    printf("url: $%s$\n",url);
    sockfd = get_request(url, port_n);

    struct pollfd fds[1];
    fds[0].fd = sockfd;
    fds[0].events = POLLIN;
    
    int timeout = 3000;
    int p=poll(fds, 1, timeout);
    if(p==0){
        printf("Timeout");
        close(sockfd);
        exit(1);
    }
    else if(p<0){
        printf("Error");
        close(sockfd);
        exit(1);
    }
    
    memset(buffer, 0, sizeof(buffer));
    ret = recv(sockfd, buffer, MAX_SIZE, 0);
    // printf("ret: %d buffer1: $%s$\n",ret,buffer);
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
            return;
        }
    }

    memset(&buffer, 0, sizeof(buffer));
    ret = recv(sockfd, buffer, MAX_SIZE, 0);
    if (ret < 0)
    {
        printf("Error receiving HTTP header!\n");
    }
    else
    {
        // printf("buffer2: %s\n", buffer);

        if (parseHeader(buffer) == 0)
        {
            send(sockfd, status_ok, strlen(status_ok), 0);
        }
        else
        {
            printf("Error in HTTP header!\n");
            close(sockfd);
            return;
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
    fileptr = fopen(file_name, "w");
    if (fileptr == NULL)
    {
        printf("Error opening file!\n");
        close(sockfd);
        return;
    }
    else
        printf("opened file!\n");

    memset(&buffer, 0, sizeof(buffer));
    while (recv(sockfd, buffer, MAX_SIZE, 0) > 0)
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
    close(sockfd);

    openFile();
}

void put_func(){
    struct sockaddr_in addr, cl_addr;
    int sockfd, ret;
    struct hostent *server;
    char *temp;
    int portNumber;

    printf("url: $%s$\n",url);
    contentFileType = (char *)calloc(100, sizeof(char));
    contentFileType = getFileType(file_name);
    
    char status_ok[] = "OK";
    char buffer[MAX_SIZE];
    char http_not_found[] = "HTTP/1.0 404 Not Found";
    char http_ok[] = "HTTP/1.0 200 OK";
    char location[] = "Location: ";
    char contentType[] = "Content-Type: ";
    int sPos, ePos;

    printf("url: $%s$\n",url);
    //  ./client GET 127.0.0.1/folder/TimeTable_Sem6.pdf
    portNumber = atoi(port_n);

    // checking the protocol specified
    if ((temp = strstr(url, "http://")) != NULL)
    {
        url = url + 7;
    }
    else if ((temp = strstr(url, "https://")) != NULL)
    {
        url = url + 8;
    }

    // checking the port number
    if (portNumber > 65536 || portNumber < 0)
    {
        printf("Invalid Port Number!");
        exit(1);
    }

    printf("url: $%s$\n",url);
    sockfd = put_request(url, port_n);

    struct pollfd fds[1];
    fds[0].fd = sockfd;
    fds[0].events = POLLIN;
    
    int timeout = 3000;
    int p=poll(fds, 1, timeout);
    if(p==0){
        printf("Timeout");
        close(sockfd);
        exit(1);
    }
    else if(p<0){
        printf("Error");
        close(sockfd);
        exit(1);
    }

    if ((fileptr = fopen(file_name, "r")) == NULL)
    {
        printf("File not found!\n");
        send(sockfd, http_not_found, strlen(http_not_found), 0); // sends HTTP 404
    }
    else
    {
        printf("Sending the file...\n");

        // http_ok = (char *)calloc(10000, sizeof(char));
        // strcpy(http_ok, "HTTP/1.0 200 OK");
        printf("$%s$\n", http_ok);
        // printf("sockfd:%d\n",sockfd);  // file descriptor is changing
        ssize_t bytes_sent = send(sockfd, http_ok, strlen(http_ok) + 1, 0); // sends HTTP 200 OK
        printf("bytes_sent: %zd, length of http_ok: %lu\n", bytes_sent, strlen(http_ok));

        memset(buffer, 0, MAX_SIZE);
        recv(sockfd, buffer, MAX_SIZE, 0);
        printf("$%s$\n", buffer);
        if ((temp = strstr(buffer, "OK")) == NULL)
        {
            printf("Operation aborted by the user!\n");
            return;
        }

        time_t timenow;
        header = (char *)calloc(MAX_SIZE, sizeof(char));
        time(&timenow);
        struct tm *timeinfo = localtime(&timenow);

        strcpy(host, "127.0.0.1");
        //strcpy(host, "127.0.0.1");
        sprintf(header, "Date: %sHostname: %s:%s\nLocation: %s\nContent-Type: %s\n\n", asctime(timeinfo), host, port_n, file_name, contentType);


        send(sockfd, header, strlen(header), 0); // sends the header
        recv(sockfd, buffer, MAX_SIZE, 0);
        if ((temp = strstr(buffer, "OK")) == NULL)
        {
            printf("Operation aborted by the user!\n");
            return;
        }
        memset(&buffer, 0, sizeof(buffer));
        while (!feof(fileptr))
        { // sends the file
            fread(&buffer, sizeof(buffer), 1, fileptr);
            send(sockfd, buffer, sizeof(buffer), 0);
            memset(&buffer, 0, sizeof(buffer));
        }
        printf("File sent...\n");
    }
    return;
}

int get_request(char *url, char *port)
{
    int sockfd, bindfd;
    char *ptr, *host;
    char getrequest[1024];
    struct sockaddr_in addr;

    if (isValidIP(url))
    { // when an IP address is given
        sprintf(getrequest, "GET / HTTP/1.0\nHOST: %s\nCONNECTION: close\nDATE:%s\nACCEPT:%s\nACCEPT-LANGUAGE: en-us\nIF-MODIFIED-SINCE:%s\nCONTENT-LANGUAGE:en-us\nCONTENT-TYPE:%s\n\n", url, get_date(),contentFileType,get_date_mod(),contentFileType);
    }
    else
    { // when a host name is given
        if ((ptr = strstr(url, "/")) == NULL)
        {
            // when hostname does not contain a slash
            sprintf(getrequest, "GET / HTTP/1.0\nHOST: %s\nCONNECTION: close\nDATE:%s\nACCEPT:%s\nACCEPT-LANGUAGE: en-us\nIF-MODIFIED-SINCE:%s\nCONTENT-LANGUAGE:en-us\nCONTENT-TYPE:%s\n\n", url, get_date(),contentFileType,get_date_mod(),contentFileType);
        }
        else
        {
            // when hostname contains a slash, it is a path to file
            strcpy(path, ptr);
            host = strtok(url, "/");

            // printf("path: $%s$\n",path);
            // printf("url: $%s$\n",url);
            sprintf(getrequest, "GET %s HTTP/1.0\nHOST: %s\nCONNECTION: close\nDATE:%s\nACCEPT:%s\nACCEPT-LANGUAGE: en-us\nIF-MODIFIED-SINCE:%s\nCONTENT-LANGUAGE:en-us\nCONTENT-TYPE:%s\n\n", path, url, get_date(),contentFileType,get_date_mod(),contentFileType);
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
    send(sockfd, getrequest, strlen(getrequest), 0);
    return sockfd;
}

int put_request(char *url, char *port){
    return 0;
}

int isValidIP(char *ip)
{
    struct sockaddr_in addr;
    int valid = inet_pton(AF_INET, ip, &(addr.sin_addr));
    return valid != 0;
}
// parse the header and return 1 if there is an error



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
    strcpy(fileName, file_name);
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
        // system(command);
        runExtCmd(command);
    }
    else if ((temp = strstr(contentFileType, "application/pdf")) != NULL)
    {
        sprintf(command, "open -a Preview.app %s", fileName);
        // system(command);
        runExtCmd(command);
    }
    else if ((temp = strstr(contentFileType, "image/jpeg")) != NULL)
    {
        sprintf(command, "open -a Preview.app %s", fileName);
        // system(command);
        runExtCmd(command);
    }
    else
    {
        printf("The filetype %s is not supported. Failed to open %s!\n", contentFileType, fileName);
    }
}

char *get_filename(char *url_var){
    char *ptr, *host_var, *ans;
    char pth[1024];
    if ((ptr = strstr(url_var, "/")) == NULL)
    {
        // when hostname does not contain a slash
        printf("no slash");
        return url_var;
    }
    else
    {
        // when hostname contains a slash, it is a pth to file
        char *fname;
        fname = (char *)calloc(1024, sizeof(char));
        ans = (char *)calloc(1024, sizeof(char));
        strcpy(pth, ptr);
        host_var = strtok(url_var, "/");
        while (host_var != NULL)
        {   
            strcpy(fname, host_var);
            // printf("host_var: %s\n", host_var);
            host_var = strtok(NULL, "/");
        }
        fname = strtok(fname, ":");
        strcpy(ans, fname);
        fname = strtok(NULL, ":");
        if(fname!=NULL){
            strcpy(port_n, fname);
        }
        // printf("pth: %s", fname);
        return ans;
    }
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

char **get_arg(char *cmd){
    char cmd_copy[1000] = {0};
    strcpy(cmd_copy, cmd);
    char **arg;

    char *p = strtok(cmd, " ");
    int cnt = 0;
    while (p){
        p = strtok(NULL, " ");
        cnt++;
    }
    arg = (char **)calloc((cnt + 1), sizeof(char *));
    p = strtok(cmd_copy, " ");
    int i = 0;
    for (; p; i++)
    {
        arg[i] = (char *)calloc((strlen(p) + 1), sizeof(char));
        strcpy(arg[i], p);
        arg[i][strlen(p)] = '\0';
        p = strtok(NULL, " ");
    }

    arg[i] = NULL;
    return arg;
}

int runExtCmd(char *usr_cmd){
    char **arg = get_arg(usr_cmd);
    execvp(arg[0], arg);
    return 0;
}
char* get_date(){
    time_t t = time(NULL);
    struct tm tm = *localtime(&t);
    char *ans;
    ans = (char *)calloc(200, sizeof(char));
    // printf("%d-%d-%d %d:%d:%d", tm.tm_year + 1900, tm.tm_mon + 1, tm.tm_mday, tm.tm_hour, tm.tm_min, tm.tm_sec);
    sprintf(ans, "%d-%d-%d %d:%d:%d", tm.tm_year + 1900, tm.tm_mon + 1, tm.tm_mday, tm.tm_hour, tm.tm_min, tm.tm_sec);
    return ans;
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

//get two day previous date 
char *get_date_mod(){
    time_t t = time(NULL);
    struct tm tm = *localtime(&t);
    char *ans;
    ans = (char *)calloc(200, sizeof(char));
    // printf("%d-%d-%d %d:%d:%d", tm.tm_year + 1900, tm.tm_mon + 1, tm.tm_mday, tm.tm_hour, tm.tm_min, tm.tm_sec);
    sprintf(ans, "%d-%d-%d %d:%d:%d", tm.tm_year + 1900, tm.tm_mon + 1, tm.tm_mday-2, tm.tm_hour, tm.tm_min, tm.tm_sec);
    return ans;
}
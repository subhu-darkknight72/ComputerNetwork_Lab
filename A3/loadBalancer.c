#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include <poll.h>
#include <time.h>

const int BUF_SIZE = 1001; //	Packet size for transmission
const int MAX_SIZE = 1001; //	Max length of strings to be ahndle in operations
void sendStr(char *str, int socket_id);
void receiveStr(char *str, int socket_id);

int main(int argc, char *argv[])
{
	int clilen;
	int sockfd, newsockfd; /* Socket descriptors */
	int sockfd_s[2];
	struct sockaddr_in cli_addr, serv_addr, serv1_addr, serv2_addr;

	int s_port[2];
	s_port[0] = atoi(argv[2]);
	s_port[1] = atoi(argv[3]);
	int c0_port = atoi(argv[1]);

	char buf[MAX_SIZE]; /* We will use this buffer for communication */

	if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0)
	{
		printf("Cannot create client-socket\n");
		exit(0);
	}

	serv_addr.sin_family = AF_INET;
	serv_addr.sin_addr.s_addr = INADDR_ANY;
	serv_addr.sin_port = htons(c0_port);

	if (bind(sockfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0)
	{
		printf("Unable to bind local address\n");
		exit(0);
	}
	else
		printf("Binded client-side address successfully!!\n");

	listen(sockfd, 5);

	while (1)
	{
		if ((sockfd_s[0] = socket(AF_INET, SOCK_STREAM, 0)) < 0)
		{
			printf("Cannot create server-socket-1\n");
			exit(0);
		}

		serv1_addr.sin_family = AF_INET;
		inet_aton("127.0.0.1", &serv1_addr.sin_addr);
		serv1_addr.sin_port = htons(s_port[0]);

		if ((connect(sockfd_s[0], (struct sockaddr *)&serv1_addr, sizeof(serv1_addr))) < 0){
			perror("Unable to connect to server-1\n");
			exit(0);
		}
		else
			printf("Connected to server-1 successfully!!\n");
		int l0, l1;

		strcpy(buf, "load");
		sendStr(buf, sockfd_s[0]);
		receiveStr(buf, sockfd_s[0]);
		l0 = atoi(buf);
		close(sockfd_s[0]);

		if ((sockfd_s[1] = socket(AF_INET, SOCK_STREAM, 0)) < 0){
			printf("Cannot create server-socket-2\n");
			exit(0);
		}

		serv2_addr.sin_family = AF_INET;
		inet_aton("127.0.0.1", &serv2_addr.sin_addr);
		serv2_addr.sin_port = htons(s_port[1]);

		if ((connect(sockfd_s[1], (struct sockaddr *)&serv2_addr, sizeof(serv2_addr))) < 0){
			perror("Unable to connect to server-2\n");
			exit(0);
		}
		else
			printf("Connected to server-2 successfully!!\n");
		
		strcpy(buf, "load");
		sendStr(buf, sockfd_s[1]);
		receiveStr(buf, sockfd_s[1]);
		l1 = atoi(buf);
		close(sockfd_s[1]);

		printf("Server1 Load: %d\n", l0);
		printf("Server2 Load: %d\n", l1);

		int timeout = 5000;
		while(timeout>0){
			struct pollfd fdset[1];
			fdset[0].events = POLLIN;	fdset[0].fd = sockfd;

			time_t t1 = time(NULL);
			int pval = poll(fdset, 1, timeout);
			time_t t2 = time(NULL);
			timeout = difftime(t2, t1);

			if(pval <0)
				printf("Error in Polling!!");
			else if(pval>0){
				clilen = sizeof(cli_addr);
				newsockfd = accept(sockfd, (struct sockaddr *)&cli_addr,(socklen_t *)&clilen);
				printf("Client Connected!!\n");

				int k = 0;
				if (l1 < l0)	k = 1;
				printf("min loaded-server: %d\n",k);

				if(fork()==0){
					close(sockfd);
					//	communicate connect
					if ((sockfd_s[k] = socket(AF_INET, SOCK_STREAM, 0)) < 0){
						printf("Cannot create server-socket-1\n");
						exit(0);
					}

					serv1_addr.sin_family = AF_INET;
					inet_aton("127.0.0.1", &serv1_addr.sin_addr);
					serv1_addr.sin_port = htons(s_port[k]);

					if ((connect(sockfd_s[k], (struct sockaddr *)&serv1_addr, sizeof(serv1_addr))) < 0){
						perror("Unable to connect to server\n");
						exit(0);
					}
					
					strcpy(buf, "time");
					sendStr(buf, sockfd_s[k]);
					receiveStr(buf, sockfd_s[k]);
					
					printf("%s\n",buf);	
					sendStr(buf, newsockfd);
					
					close(sockfd_s[k]);
					close(newsockfd);
					exit(0);
				}
				close(newsockfd);
			}


		}
	}
	close(sockfd_s[0]);
	close(sockfd_s[1]);
	close(sockfd);
	return 0;
}

/*
	takes a string "str" (may be long), and send it to the client
	by splitting the string into small chunks.
*/
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

/*
	receives string "str" (may be long) from the client
	by concatenating the incoming string chunk stream
*/
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
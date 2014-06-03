/*************************************************************************
    > File Name: pmanager.cc
    > Author: zang
    > Usage: place management of SPANStore
    > Mail: zangzhida@gmail.com 
    > Created Time: Tue May  6 20:19:08 2014
 ************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <string.h>
#include <string>
#include "lock_server.h"
#define MAXM 15
#define MAXBUF 64
#define handle_error(msg)\
	do {\
		perror(msg);\
		exit(EXIT_FAILURE);\
	}while(0)

using namespace std;

/*
 * Show how to use this program
 */
void usage(char *name);

/*
 * Start daemon that receive heartbeat of servers
 */
void *startudp(void *ip);

/*
 * set replica policy
 */
void setpolicy();

/*
 * transfer replica policy to server
 */
void transferpolicy(int port);

void *setfalse(void *);

char ips[15][16];	//store ip of the servers
int n;	//the number of servers, should be larger than 0 and smaller than 16

int m;	//initial data number
int ids[MAXM];	//data's id
char policy[MAXM][16];	//replica policy of the initial m data

bool alive[MAXM] = {false};

struct ipinfo {
	int port;
	string ip;
};

lock_server ls;

int main(int argc, char **argv)
{
	if (argc != 3) {
		char *name = argv[0];
		usage(name);
		exit(1);
	}

	setpolicy();

	struct ipinfo mip;
	mip.port = atoi(argv[2]);
	mip.ip = argv[1];
	pthread_t pid;
	pthread_create(&pid, NULL, startudp, (void *)&mip);

	pthread_t ps;
	pthread_create(&ps, NULL, setfalse, NULL);

	printf("Now please start the servers and then input 'y'\n");
	char answer;
	while ((answer = getchar()) != 'y') {
	}

	printf("Please input the servers port:");
	int tmp;
	while (scanf("%d", &tmp) != 1);

	printf("Now begin to transfer policy\n\n");

	transferpolicy(tmp);

	for (int i = 0; i < n; i++)
		printf("#%dth ip: %s\n", i+1, ips[i]);

	for (int i = 0; i < m; i++)
		printf("#%dth policy: %d %s\n\n", i+1, ids[i], policy[i]);

	while (1) {
		printf("Set new replica policy? 'y'\n");

		while ((answer = getchar()) != 'y')
			;

		printf("******Input new replica policy******\n");
		setpolicy();

		printf("Now begin to transfer policy\n");
		transferpolicy(tmp);
	}

	int *ev;
	pthread_join(pid, (void **)&ev);
	pthread_join(ps, NULL);
	
	return 0;
}

void usage(char *name)
{
	printf("******Notice******\n");
	printf("* Usage: %s ip port\n", name);
	printf("* ip port used to receive heartbeat\n");
	printf("******************\n");
}

void *startudp(void *ip)
{
	struct ipinfo *tmpip = (struct ipinfo *)ip;
	struct sockaddr_in addr;
	addr.sin_family = AF_INET;
	addr.sin_port = htons(tmpip->port);
	addr.sin_addr.s_addr = htonl(INADDR_ANY);

	int sock;
	if ((sock = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
		perror("socket error!");
		exit(1);
	}
	if (bind(sock, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
		perror("bind error!");
		exit(1);
	}

	char buf[4];
	struct sockaddr_in clientAddr;
	socklen_t len = sizeof(struct sockaddr_in);
	while (1) {
		recvfrom(sock, buf, 3, 0, (struct sockaddr*)&clientAddr, &len);
		char *tmpip = inet_ntoa(clientAddr.sin_addr);
		for (int i = 0; i < n; i++) {
			if (!strncmp(tmpip, ips[i], 16))
				ls.acquire(i);
				alive[i] = true;
				ls.release(i);
		}
	}
}

void setpolicy()
{
	printf("Input server number n (0< n <16)\n");

	printf("n=");
	scanf("%d", &n);
	while (n < 1 || n > 15) {
		printf("n is not legal, input again!\n");
		scanf("%d", &n);
	}
	
	printf("Now input %d ip(one line one ip)\n", n);

	for (int i = 0; i < n; i++) {
		printf("#%d:", i+1);
		scanf("%s", ips[i]);
	}

	char tmp = 'A' + n - 1;

	printf("Input initial data number m (0< m <16)\n");

	printf("m=");
	scanf("%d", &m);
	while (m < 1 || m > 15) {
		printf("m is not legal, input again!\n");
		scanf("%d", &m);
	}

	printf("Now input %d data and its policy (server ID should be A to %c)\n", m, tmp);

	for (int i = 0; i < m; i++) {
		printf("#%d:", i+1);
		scanf("%d %s", &ids[i], policy[i]);

		//make sure the replica server in the policy is among the setted servers
		char t;
		while (1) {
			bool isvalid = true;
			char *p = policy[i];
			while (t = *p++) {
			 	if (t > tmp || t < 'A') {
					printf("server ID error, input policy again\n");
					isvalid = false;
				 	break;
				} 
			}
			if (isvalid)
				break;
			scanf("%s", policy[i]);
		}
	}

	printf("\n");
}

void transferpolicy(int port)
{
	char buf[MAXBUF];
	int sockfd;
	socklen_t len;
	struct sockaddr_in saddr;
	int ret;

	for (int i = 0; i < m; i++) {
		bzero(buf, MAXBUF + 1);
		snprintf(buf, MAXBUF, "%d:", ids[i]);
		strcat(buf, policy[i]);
		char *tmp = policy[i];
		while (*tmp) {
			bzero(&saddr, sizeof(saddr));
			saddr.sin_port = htons(port);
			saddr.sin_family = AF_INET;

			int k = *tmp - 'A';
			if (!alive[k]) {
				tmp++;
				continue;
			}
			string ip(ips[k]);
			
			saddr.sin_addr.s_addr = inet_addr(ip.c_str());

			if (-1 == (sockfd = socket(AF_INET, SOCK_STREAM, 0)))
				handle_error("create socket error!");

			len = sizeof(saddr);
			if (-1 == (ret = connect(sockfd, (struct sockaddr*)&saddr, len)))
				handle_error("connect error!");

			if (0 == (len = send(sockfd, buf, sizeof(buf) - 1, 0)))
				printf("send error!\n");

			close(sockfd);

			tmp++;
		}
		printf("\n");
	}
}

void *setfalse(void *)
{
	sleep(13);
	for (int i = 0; i < n; i++) {
		ls.acquire(i);
		alive[i] = false;
		ls.release(i);
	}
}

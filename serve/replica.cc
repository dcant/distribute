/*************************************************************************
    > File Name: replica.cc
    > Author: zang
    > Usage: metadata server of SPANStore
    > Mail: zangzhida@gmail.com 
    > Created Time: Mon May 12 21:01:10 2014
 ************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <errno.h>
#include <map>
#include <sys/types.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/wait.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/time.h>
#include <string.h>
#include <string>
#define MAXBUF 64
#define handle_error(msg)\
	do {\
		perror(msg);\
		exit(EXIT_FAILURE);\
	} while (0)

using namespace std;

void usage(char *name);

/*
 * start serve as metadata server.
 */
void start_service(void *arg);

/*
 * send heartbeat to pmanager.
 */ 
void *start_heartbeat(void *arg);

/*
 * metadata used to store data's replica policy
 */ 
map<int, string> metadata;
struct ipinfo {
	int port;
	string ip;
};


int main(int argc, char **argv)
{
	if (argc < 5) {
		usage(argv[0]);
		exit(1);
	}

	pthread_t pid;
	struct ipinfo sip;
	sip.port = atoi(argv[4]);
	sip.ip = argv[3];
	pthread_create(&pid, NULL, start_heartbeat, (void *)&sip);

	struct ipinfo cip;
	cip.port = atoi(argv[2]);
	cip.ip = argv[1];
	start_service((void *)&cip);

	pthread_join(pid, NULL);
}

void usage(char *name)
{
	printf("******Notice******\n");
	printf("* Usage: %s selfip port1 pmanip port2\n", name);
	printf("* port1 used to receive metadata, port2 used to send heartbeat.\n");
	printf("******************\n");
}

void start_service(void *arg)
{
	struct ipinfo *tmpip = (struct ipinfo *)arg;
	int sockfd, nfd;
	socklen_t len;
	struct sockaddr_in maddr, caddr;
	int ret;
	char buf[MAXBUF];
	int id;
	string policy;

	if (-1 == (sockfd = socket(AF_INET, SOCK_STREAM, 0)))
		handle_error("create socket error!");
	
	bzero(&maddr, sizeof(maddr));
	maddr.sin_family = AF_INET;
	maddr.sin_port = htons(tmpip->port);
	maddr.sin_addr.s_addr = inet_addr(tmpip->ip.c_str());//htonl(INADDR_ANY);

	if (-1 == bind(sockfd, (struct sockaddr *)&maddr, sizeof(maddr)))
		handle_error("bind error!");

	if (-1 == listen(sockfd, 5))
		handle_error("listen error!");

	len = sizeof(caddr);
	while (1) {
		if (-1 == (nfd = accept(sockfd, (struct sockaddr *)&caddr, &len)))
			handle_error("accept error!");
		bzero(buf, sizeof(buf));
		recv(nfd, buf, MAXBUF, 0);

		char *tmp = strchr(buf, ':');
		*tmp = '\0';
		tmp++;
		id = atoi(buf);
		policy = string(tmp);

		map<int, string>::iterator it = metadata.find(id);
		if ( it != metadata.end())
			metadata[id] = policy;
		else
			metadata.insert(pair<int, string>(id, policy));

		printf("replica policy: %d => %s\n", id, tmp);
	}
}

void *start_heartbeat(void *arg)
{
	struct ipinfo *tmpip = (struct ipinfo *)arg;
	while (1) {
		struct sockaddr_in addr;
		int sockfd;
		if (-1 == (sockfd = socket(AF_INET, SOCK_DGRAM, 0)))
			handle_error("socket error!");
		addr.sin_family = AF_INET;
		addr.sin_port = htons(tmpip->port);
		addr.sin_addr.s_addr == inet_addr(tmpip->ip.c_str());

		char buf[MAXBUF];
		int len = sizeof(addr);
		buf[0] = 'O';
		sendto(sockfd, buf, strlen(buf), 0, (struct sockaddr *)&addr, sizeof(addr));
		sleep(2);
	}
}

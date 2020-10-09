
#include <stdio.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <fcntl.h>
#include <signal.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>

// proactor
//
//
//
int sockfd = 0;

// 异步操作 async
void do_sigio(int sig) {
	char buffer[256] = {0};
	struct sockaddr_in cliaddr;
	int clilen = sizeof(struct sockaddr_in);
	int len = recvfrom(sockfd, buffer, 256, 0, (struct sockaddr*)&cliaddr, (socklen_t*)&clilen);
	printf("Listen Message: %s\r\n", buffer);
	int slen = sendto(sockfd, buffer, len, 0, (struct sockaddr*)&cliaddr, clilen);
}


int main() {

	struct sigaction sigio_action;
	sigio_action.sa_flags = 0;
	sigio_action.sa_handler = do_sigio;
	sigaction(SIGIO, &sigio_action, NULL);

	sockfd = socket(AF_INET, SOCK_DGRAM, 0);
	struct sockaddr_in serv_addr;
	memset(&serv_addr, 0, sizeof(serv_addr));

	serv_addr.sin_family = AF_INET;
	serv_addr.sin_port = htons(6016);
	serv_addr.sin_addr.s_addr = INADDR_ANY;

	bind(sockfd, (struct sockaddr*)&serv_addr, sizeof(serv_addr));

	fcntl(sockfd, F_SETOWN, getpid());

	int flags = fcntl(sockfd, F_GETFL, 0);  // 获取io状态

	flags |= O_ASYNC | O_NONBLOCK;
	fcntl(sockfd, F_SETFL, flags);

	while(1) sleep(1);  // 
	close(sockfd);
	return 0;

}

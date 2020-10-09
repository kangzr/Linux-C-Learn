
int main() {
	int sockfd = socket(AF_INET, SOCK_STREAM, 0);
	if (sockfd < 0) return -1;

	struct sockaddr_in serv_addr;
	memset(&serv_addr, 0, sizeof(serv_addr));

	serv_addr.sin_family = AF_INET;
	serv_addr.sin_port = htons(6016);
	serv_addr.sin_addr.s_addr = INADDR_ANY;

	if (bind(sockfd, (struct sockaddr*)&serv_addr, sizeof(serv_addr))){
		// bind失败，k端口被占用	
		perror("bind");
		return -2;
	}

	// 如何理解5
	if (listen(sockfd, 5) < 0) {
		perror("listen"); return -3;
	}

	// server info
	// 两个状态，可读/可写
	fd_set rfds, rset;
}

#include <unistd.h>
#include <stdio.h>
#include <sys/socket.h>
#include <stdlib.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <string.h>

#define HOST "192.168.137.1"
#define PORT 65431

#define LEN_BUF 1024

int main(void) {
	int server_fd, new_socket;
	struct sockaddr_in addr_server, addr_client;
	int opt = 1;
	int addrlen = sizeof(struct sockaddr_in);
	char buffer[LEN_BUF] = {0};

	// Creating socket file descriptor
	if((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0) {
		perror("socket failed");
		exit(EXIT_FAILURE);
	}

	addr_server.sin_family = AF_INET;
	addr_server.sin_addr.s_addr = inet_addr(HOST);
	addr_server.sin_port = htons(PORT);

	// Forcefully attaching socket to the port PORT
	if(bind(server_fd, (struct sockaddr *)&addr_server, sizeof(addr_server)) < 0) {
		perror("bind failed");
		exit(EXIT_FAILURE);
	}
	if (listen(server_fd, 3) < 0) {
		perror("listen");
		exit(EXIT_FAILURE);
	}
	if ((new_socket = accept(server_fd, (struct sockaddr *)&addr_client, (socklen_t*)&addrlen)) < 0) {
		perror("accept");
		exit(EXIT_FAILURE);
	}
	fprintf(stdout, "starting program 'server':\n");
	while(1) {
		FILE *fp;

		fp = fopen("log.dat", "a");
		if(read(new_socket, buffer, 1024) > 0) {
			fprintf(fp, "%s", buffer);
		}
		fclose(fp);
	}
	close(server_fd);
	return 0;
}



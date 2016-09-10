#include "cfclient.h"

#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>



int cfclient_open(cfclient *client) {

	client->buf_size = 0;
	client->buf_i = 0;

	if((client->netfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
		perror("cannot create socket");
		return 1;
	}

	// The server address
	struct sockaddr_in addr;
	memset((char *)&addr, 0, sizeof(addr));
	addr.sin_family = AF_INET;
	addr.sin_addr.s_addr = htonl(INADDR_ANY);
	addr.sin_port = htons(14555);

	if (bind(client->netfd, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
		perror("bind failed");
		close(client->netfd);
		return 1;
	}

	int flags = fcntl(client->netfd, F_GETFL, 0);
	if(fcntl(client->netfd, F_SETFL, flags | O_NONBLOCK)){
		perror("failed to make udp socket non-blocking");
		close(client->netfd);
		return 1;
	}

	// By default send to 127.0.0.1:14550
	memset((char *)&client->client_addr, 0, sizeof(client->client_addr));
	client->client_addr.sin_family = AF_INET;
	client->client_addr.sin_port = htons(14550);
	inet_pton(AF_INET, "127.0.0.1", &client->client_addr.sin_addr);

	return 0;
}

int cfclient_close(cfclient *client) {
	close(client->netfd);
}

int cfclient_buffer_empty(cfclient *client) {
	return client->buf_size == 0;
}

crtp_message_t *cfclient_buffer_head(cfclient *client) {
	int i = (client->buf_i + CFCLIENT_MAX_BUFFERSIZE - client->buf_size) % CFCLIENT_MAX_BUFFERSIZE;
	return &client->buf[i];
}

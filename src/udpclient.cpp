#include "udpclient.h"

#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>


//bool UdpUri::Parse()


int UdpClient::open(int lport, int rport) {

	this->buf_size = 0;
	this->buf_i = 0;

	if((this->netfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
		perror("cannot create socket");
		return 1;
	}

	// The server address
	struct sockaddr_in addr;
	memset((char *)&addr, 0, sizeof(addr));
	addr.sin_family = AF_INET;
	addr.sin_addr.s_addr = htonl(INADDR_ANY);
	addr.sin_port = htons(lport);

	if (bind(this->netfd, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
		perror("bind failed");
		::close(this->netfd);
		return 1;
	}

	int flags = fcntl(this->netfd, F_GETFL, 0);
	if(fcntl(this->netfd, F_SETFL, flags | O_NONBLOCK)){
		perror("failed to make udp socket non-blocking");
		::close(this->netfd);
		return 1;
	}

	// Setup where messages should be sent to
	memset((char *)&this->client_addr, 0, sizeof(this->client_addr));
	this->client_addr.sin_family = AF_INET;
	this->client_addr.sin_port = htons(rport);
	inet_pton(AF_INET, "127.0.0.1", &this->client_addr.sin_addr);

	return 0;
}

int UdpClient::close() {
	::close(this->netfd);
	return 0;
}


#define MIN(X, Y) (((X) < (Y)) ? (X) : (Y))


int UdpClient::read() {

// TODO: Which memory buffer should I use

	int n = recvfrom(this->netfd, buf, 512, 0, NULL, NULL);


	for(int i = 0; i < n; i += CRTP_MAX_DATA_SIZE) {
		int size = MIN(CRTP_MAX_DATA_SIZE, n - i);

		crtp_message_t *msg = &this->buf[this->buf_i];
		msg->size = size + 1;
		msg->header = 0;
		msg->port = CRTP_PORT_MAVLINK;
		memcpy(msg->data, buf + i, size);

		// Update buffer size and offset
		this->buf_i = (this->buf_i + 1) % UDPCLIENT_MAX_BUFFERSIZE;
		if(this->buf_size < UDPCLIENT_MAX_BUFFERSIZE)
			this->buf_size++;
	}

// TODO: Return values
}

int UdpClient::write(crtp_message_t *msg) {
	sendto(this->netfd, &msg->data, msg->size - 1, 0, (const struct sockaddr *) &this->client_addr, sizeof(this->client_addr));
}

int UdpClient::buffer_empty() {
	return this->buf_size == 0;
}

crtp_message_t *UdpClient::buffer_head() {
	int i = (this->buf_i + UDPCLIENT_MAX_BUFFERSIZE - this->buf_size) % UDPCLIENT_MAX_BUFFERSIZE;
	return &this->buf[i];
}

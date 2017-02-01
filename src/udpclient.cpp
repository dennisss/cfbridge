#include "udpclient.h"

#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>

#include <regex>
#include <iostream>

using namespace std;


bool UdpClient::Parse(string uri, UdpUri &out) {

	regex reg("^udp://([0-9\\.]+)?:([0-9]+)@([0-9\\.]+)?:([0-9]+)$");
	smatch mat;

	if (!regex_match(uri, mat, reg)) {
		cerr << "Invalid udp uri: " << uri << endl;
		return false;
	}

	int res = 1;

	// Init server address
	memset((char *)&out.server_addr, 0, sizeof(out.server_addr));
	out.server_addr.sin_family = AF_INET;

	// Server interface ip address
	if(mat[1].length() > 0)
		res = inet_pton(AF_INET, mat[1].str().c_str(), &out.server_addr.sin_addr);
	else
		out.server_addr.sin_addr.s_addr = htonl(INADDR_ANY);

	if(!res) {
		printf("Invalid server ip address\n");
		return false;
	}

	// Server port
	out.server_addr.sin_port = htons( stoi(mat[2]) );


	// Init client address
	memset((char *)&out.client_addr, 0, sizeof(out.client_addr));
	out.client_addr.sin_family = AF_INET;

	// Client ip address
	if(mat[3].length() > 0)
		res = inet_pton(AF_INET, mat[3].str().c_str(), &out.client_addr.sin_addr);
	else
		res = inet_pton(AF_INET, "127.0.0.1", &out.client_addr.sin_addr);

	if(!res) {
		printf("Invalid client ip address\n");
		return false;
	}

	// Client port
	out.client_addr.sin_port = htons( stoi(mat[4]) );

	return true;
}

UdpClient::UdpClient(const UdpUri &uri) {
	this->config = uri;
	this->netfd = -1;
}

UdpClient::~UdpClient() {
	if(this->netfd > 0) {
		this->close();
	}
}


int UdpClient::open() {

	this->buf_size = 0;
	this->buf_i = 0;

	if((this->netfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
		perror("cannot create socket");
		return 1;
	}

	if (::bind(this->netfd, (struct sockaddr *)&config.server_addr, sizeof(config.server_addr)) < 0) {
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

	return 0;
}

int UdpClient::close() {
	::close(this->netfd);
	this->netfd = -1;
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

	return n;

// TODO: Return values
}

int UdpClient::write(crtp_message_t *msg) {
	return sendto(this->netfd, &msg->data, msg->size - 1, 0, (const struct sockaddr *) &config.client_addr, sizeof(config.client_addr));
}

int UdpClient::buffer_empty() {
	return this->buf_size == 0;
}

crtp_message_t *UdpClient::buffer_head() {
	int i = (this->buf_i + UDPCLIENT_MAX_BUFFERSIZE - this->buf_size) % UDPCLIENT_MAX_BUFFERSIZE;
	return &this->buf[i];
}

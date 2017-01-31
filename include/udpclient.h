#ifndef UDPCLIENT_H
#define UDPCLIENT_H

#include <stdint.h>
#include <sys/socket.h>
#include <netinet/ip.h>
#include <arpa/inet.h>

#include <crtp.h>

#include <memory>
#include <string>


#define UDPCLIENT_MAX_BUFFERSIZE 64

struct UdpUri {
	struct sockaddr_in client_addr;
	struct sockaddr_in server_addr;
};


/* Descriptor for each UDP/GCS connection */
class UdpClient {
public:

	typedef std::shared_ptr<UdpClient> Ptr;


	static bool Parse(std::string uri, UdpUri &out);

	/* Creates a new client that is listening for messages on lport and will forward messages to rport */
	int open(int lport = 14555, int rport = 14550);

	int close();


	int read();

	int write(crtp_message_t *msg);


	/**
	 * Gets the underlying file descriptor for polling purposes
	 */
	int get_fd() { return this->netfd; }

	/* Call this whenever data has been read from the UDP server */
	//int read_data(char *buf, int n);


	int buffer_empty();
	crtp_message_t *buffer_head();
	void pop_head() { this->buf_size--; }


private:
	int netfd;
	//uint64_t radio_addr;
	struct sockaddr_in client_addr; // Where messages should be sent (GCS address)
	struct sockaddr_in server_addr;
	crtp_message_t buf[UDPCLIENT_MAX_BUFFERSIZE]; // Messages received from GCS which should be sent to usb
	int buf_i;
	int buf_size;
};



#endif

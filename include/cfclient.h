
#include <stdint.h>
#include <sys/socket.h>
#include <netinet/ip.h>
#include <arpa/inet.h>

#include <crtp.h>


#define CFCLIENT_MAX_BUFFERSIZE 64

/* Descriptor for each UDP/GCS connection */
typedef struct {
	int netfd;
	uint64_t radio_addr;
	struct sockaddr_in client_addr; // Where messages should be sent (GCS address)
	struct sockaddr_in server_addr;
	crtp_message_t buf[CFCLIENT_MAX_BUFFERSIZE]; // Messages received from GCS which should be sent to usb
	int buf_i;
	int buf_size;
} cfclient;


int cfclient_open(cfclient *client);
int cfclient_close(cfclient *client);

int cfclient_buffer_empty(cfclient *client);
crtp_message_t *cfclient_buffer_head(cfclient *client);

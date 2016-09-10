#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <poll.h>
#include <signal.h>

#include "cfclient.h"
#include "cfradio.h"


static libusb_context *ctx;

static bool running;

static cfradio radio;
static cfclient client;


void signal_sigint(int s) {
	running = false;
}


// Grabs a message from the client buffer if there are any
int fetch_message(crtp_message_t *buf) {
	if(cfclient_buffer_empty(&client))
		return 0;

	crtp_message_t *msg = cfclient_buffer_head(&client);
	client.buf_size--;

	// TODO: This could be more efficient
	memcpy(buf, msg, sizeof(crtp_message_t));
	return 1;
}

int handle_message(int status, crtp_message_t *msg) {
	if(msg->port != CRTP_PORT_MAVLINK)
		return 0;

	// Forward to udp
	sendto(client.netfd, &msg->data, msg->size - 1, 0, &client.client_addr, sizeof(client.client_addr));
	return 1;
}


#define MIN(X, Y) (((X) < (Y)) ? (X) : (Y))


int main(int argc, char *argv[]) {
	int res = 0;


	if(libusb_init(&ctx) != 0) {
		printf("Failed to init libusb\n");
		return 1;
	}


	if(cfradio_open(&radio, ctx, fetch_message, handle_message) != 0) {
		return 1;
	}

	if(cfclient_open(&client) != 0) {
		return 1;
	}


	// Intercept ctrl-c for gracefully quiting
	running = true;
	signal(SIGINT, signal_sigint);


	struct pollfd fds[16]; // TODO: We won't need all of them, but I don't know how many libusb needs
	int nfds = 1;

	// First descriptor for udp socket
	fds[0].fd = client.netfd;
	fds[0].events = POLLIN; // TODO: Set netfd to non-blocking and allow the OS to buffer the sendto if needed

	// Rest for usb
	const struct libusb_pollfd **usbfds = libusb_get_pollfds(ctx);
	for(int i = 0; usbfds[i] != NULL; i++) {
		fds[nfds].fd = usbfds[i]->fd;
		fds[nfds].events = usbfds[i]->events;
		nfds++;
	}
	//libusb_free_pollfds(usbfds);

	struct timeval tv;
	memset(&tv, 0, sizeof(tv));


	char *buf = (char *) malloc(512); // Received from socket

	printf("Waiting for messages...\n");

	while(running) {
		res = poll(fds, nfds, 50);
		if(res < 0) {
			// Error
			continue;
		}
		else if(res == 0) {
			// timeout
			cfradio_notify(&radio);
			continue;
		}


		// Handling udp transfers
		if(fds[0].revents) {
			if(fds[0].revents & POLLIN) {
				int res = recvfrom(client.netfd, buf, 512, 0, NULL, NULL);

				if(res > 0) {

					for(int i = 0; i < res; i += CRTP_MAX_DATA_SIZE) {
						int size = MIN(CRTP_MAX_DATA_SIZE, res - i);

						crtp_message_t *msg = &client.buf[client.buf_i];
						msg->size = size + 1;
						msg->header = 0;
						msg->port = CRTP_PORT_MAVLINK;
						memcpy(msg->data, buf + i, size);

						// Update buffer size and offset
						client.buf_i = (client.buf_i + 1) % CFCLIENT_MAX_BUFFERSIZE;
						if(client.buf_size < CFCLIENT_MAX_BUFFERSIZE)
							client.buf_size++;
					}

					cfradio_notify(&radio);
				}

			}
			res--;
		}


		// The rest must be usb fds
		if(res > 0) {
			libusb_handle_events_timeout_completed(ctx, &tv, NULL);
		}
	}

	printf("Releasing...\n");

	free(buf);

	cfclient_close(&client);

	cfradio_close(&radio);

//	libusb_exit(ctx);

	printf("Done!\n");
}

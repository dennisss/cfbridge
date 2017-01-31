
#include "bridge.h"

#include <stdlib.h>
#include <string.h>
#include <poll.h>

// TODO: These also need to know which radio called them in order to pick from the correct group of radios
// Grabs a message from a client buffer if there are any
static int fetch_message(RadioUri *uri, crtp_message_t *buf, void *arg) {

	Bridge *b = (Bridge *) arg;

	if(b->clients[0]->buffer_empty())
		return 0;

	crtp_message_t *msg = b->clients[0]->buffer_head();
	b->clients[0]->pop_head();

	// TODO: This could be more efficient
	memcpy(buf, msg, sizeof(crtp_message_t));
	return 1;
}

static int handle_message(int status, crtp_message_t *msg, void *arg) {
	Bridge *b = (Bridge *) arg;

	if(msg->port != CRTP_PORT_MAVLINK)
		return 0;

	// Forward to udp
	b->clients[0]->write(msg);
	return 1;
}


void Bridge::run() {


	struct pollfd fds[32]; // TODO: We won't need all of them, but I don't know how many libusb needs
	int nfds = 0;

	// The first set of file descriptors will be for the clients
	for(int i = 0; i < connections.size(); i++) {
		for(int j = 0; j < connections[i].size(); j++) {
			int n = nfds++;
			fds[n].fd = connections[i][j].client->get_fd();
			fds[n].events = POLLIN; // TODO: Set netfd to non-blocking and allow the OS to buffer the sendto if needed
		}
	}

	// Rest for usb
	const struct libusb_pollfd **usbfds = libusb_get_pollfds(ctx);
	for(int i = 0; usbfds[i] != NULL; i++) {
		fds[nfds].fd = usbfds[i]->fd;
		fds[nfds].events = usbfds[i]->events;
		nfds++;
	}
	// TODO: Check why this isn't working
	//libusb_free_pollfds(usbfds);

	struct timeval tv;
	memset(&tv, 0, sizeof(tv));


	char *buf = (char *) malloc(512); // Received from socket
	int res;

	printf("Waiting for messages...\n");

	while(running) {
		res = poll(fds, nfds, 50);
		if(res < 0) {
			// Error
			continue;
		}
		else if(res == 0) {
			// timeout

			// TODO: Maintain a timer for each radio
			radios[0]->notify();
			continue;
		}


		// Handling udp transfers
		if(fds[0].revents) {
			if(fds[0].revents & POLLIN) {

				if(clients[0]->read(buf, res) > 0) {
					radios[0]->notify();
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


}

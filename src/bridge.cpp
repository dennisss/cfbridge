
#include "bridge.h"

#include <stdlib.h>
#include <string.h>
#include <poll.h>
#include <signal.h>

using namespace std;

static bool running;
void signal_sigint(int s) {
	running = false;
}

// Grabs a message from a client buffer if there are any
int bridge_fetch_message(RadioUri *uri, crtp_message_t *buf, void *arg) {

	Bridge *b = (Bridge *) arg;
	int ri = b->radio_numbers[uri->num];

	// Simple scheduling (ping different client each time)
	// TODO: Instead, we should only switch once a full mavlink message has been sent
	// TODO: Idle mode clients will need a separate scheduling policy as well
	b->active_clients[ri] = (b->active_clients[ri] + 1) % b->connections[ri].size();

	int ci = b->active_clients[ri];
	BridgeConnection &c = b->connections[ri][ci];

	// Switch the radio to the current client
	*uri = c.config;

	if(c.client->buffer_empty())
		return 0;

	crtp_message_t *msg = c.client->buffer_head();
	c.client->pop_head();

	// TODO: This could be more efficient
	memcpy(buf, msg, sizeof(crtp_message_t));
	return 1;
}

int bridge_handle_message(int status, RadioUri *uri, crtp_message_t *msg, void *arg) {
	Bridge *b = (Bridge *) arg;
	int ri = b->radio_numbers[uri->num];
	int ci = b->active_clients[ri];
	BridgeConnection &c = b->connections[ri][ci];

	if(msg->port != CRTP_PORT_MAVLINK)
		return 0;

	// Forward to udp
	c.client->write(msg);
	return 1;
}


Bridge::Bridge(libusb_context *ctx, vector<Crazyradio::Ptr> radios, vector<vector<BridgeConnection>> connections) {

	this->ctx = ctx;

	this->radios = radios;

	for(auto radio : radios) {
		radio->set_callbacks(bridge_fetch_message, bridge_handle_message, this);
	}

	this->connections = connections;


	// Initialize all radios to first client
	active_clients.resize(radios.size(), 0);
	for(int i = 0; i < radios.size(); i++) {
		radios[i]->set_config(connections[i][0].config);

		radio_numbers[radios[i]->get_number()] = i;
	}

}



void Bridge::run() {

	// Intercept ctrl-c for gracefully quiting
	running = true;
	signal(SIGINT, signal_sigint);


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
	libusb_free_pollfds(usbfds);

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
			// Then if any of them timeout beyond the idle time, we should do this
			for(auto radio : radios) {
				radio->notify();
			}
			continue;
		}


		// Handling udp transfers
		int n = 0;
		for(int i = 0; i < connections.size(); i++) {
			for(int j = 0; j < connections[i].size(); j++) {
				BridgeConnection &c = connections[i][j];

				if(fds[n].revents) {
					if(fds[n].revents & POLLIN) {

						if(c.client->read() > 0) {
							int n = c.config.num;
							radios[n]->notify(); // TODO: THis assumes that there are no gaps in the radio array
						}

					}
					res--;
				}

				n++;
			}
		}


		// The rest must be usb fds
		if(res > 0) {
			libusb_handle_events_timeout_completed(ctx, &tv, NULL);
		}
	}

	printf("Releasing...\n");

	free(buf);


}

#ifndef CFBRIDGE_H
#define CFBRIDGE_H

#include "crazyradio.h"
#include "udpclient.h"

#include <vector>
#include <map>

/**
 * Essentially represents the intent to connect a single Crazyflie through a radio in the bridge
 */
struct BridgeConnection {
	UdpClient::Ptr client;
	RadioUri config;
};

/**
 *
 */
class Bridge {
public:

	Bridge(libusb_context *ctx, std::vector<Crazyradio::Ptr> radios, std::vector<std::vector<BridgeConnection>> connections);

	void start();
	void stop();

	void run();

private:

	friend int bridge_fetch_message(RadioUri *uri, crtp_message_t *buf, void *arg);
	friend int bridge_handle_message(int status, RadioUri *uri, crtp_message_t *msg, void *arg);

	libusb_context *ctx;

	// Map from radio number to index in the radios array
	std::map<int, int> radio_numbers;

	//
	std::vector<Crazyradio::Ptr> radios;
	std::vector<uint64_t> last_sent;

	// For each radio, this is the index of the currently configured connection
	// To manage multiple, we will cycle through and send each one
	// TODO: We shouldn't cycle until after an entire mavlink message is sent, otherwise the address switch becomes inefficient
	std::vector<int> active_clients;

	// Bridge connections organized by which radio will be responsible for them
	std::vector<std::vector<BridgeConnection>> connections;

};


int bridge_fetch_message(RadioUri *uri, crtp_message_t *buf, void *arg);
int bridge_handle_message(int status, RadioUri *uri, crtp_message_t *msg, void *arg);



#endif

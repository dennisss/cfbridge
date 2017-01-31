#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <signal.h>

#include "bridge.h"
#include "udpclient.h"
#include "crazyradio.h"


#include <fstream>

#include <regex>
#include <string>
#include <iostream>

using namespace std;

static libusb_context *ctx;

static bool running;


void signal_sigint(int s) {
	running = false;
}

bool parse_client_uri(string uri) {

	regex reg("^udp://([0-9\\.]+)?:([0-9]+)@([0-9\\.]+)?:([0-9]+)$");
	smatch mat;

	if (!regex_match(uri, mat, reg)) {
		cerr << "Invalid udp uri: " << uri << endl;
		return false;
	}


	for (int i = 0; i < mat.size(); i++) {
		cout << mat[i] << endl;
	}

}

void parse_radio_uri(string uri) {
	regex reg("^radio://[0-9]+(/([0-9]+)(/(250K|1M|2M)(/([0-9A-F]{10}))?)?)?$");



}

/*
	Arguments

	- Define radios
		- How many to open

		--radios n

	- Define radio clients
		- 2 UDP ports, channel #, speed,

		--client udp://:14550@:14555 radio://0/23/2M

		THIS IS the same format as the regular
		 radio://<dongle nbr>/<radio channel>/[250K,1M,2M]/E7E7E7E7E7
*/
int main(int argc, char *argv[]) {
	int res = 0;


	vector<Crazyradio *> radios;
	vector<UdpClient *> clients;

	if(libusb_init(&ctx) != 0) {
		printf("Failed to init libusb\n");
		return 1;
	}


/*


	if(cfradio_open(&radio, ctx, fetch_message, handle_message) != 0) {
		return 1;
	}

	if(cfclient_open(&client) != 0) {
		return 1;
	}


	// Intercept ctrl-c for gracefully quiting
	running = true;
	signal(SIGINT, signal_sigint);



	cfbridge_run();





	// Cleaning up
	cfclient_close(&client);
	cfradio_close(&radio);
//	libusb_exit(ctx);

*/

	printf("Done!\n");
}

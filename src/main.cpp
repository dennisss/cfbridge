#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>

#include "bridge.h"
#include "udpclient.h"
#include "crazyradio.h"


#include <fstream>
#include <string>
#include <sstream>
#include <iostream>

using namespace std;

static libusb_context *ctx;



void usage() {
	cout << "Usage: ./bridge [config_file.txt]" << endl;
}


int main(int argc, char *argv[]) {
	int res = 0;

	if(argc > 2) {
		usage();
		return 1;
	}

	ifstream file(argc == 2? argv[1] : "default.txt");

	if(!file.is_open()) {
		cout << "Couldn't open config file: " << (argc == 2? argv[1] : "default.txt") << endl;
		return 1;
	}

	if(libusb_init(&ctx) != 0) {
		printf("Failed to init libusb\n");
		return 1;
	}


	vector<Crazyradio::Ptr> radios = Crazyradio::Enumerate(ctx);
	if(radios.size() == 0) {
		cout << "No radios are connected!" << endl;
		return 1;
	}

	vector<vector<BridgeConnection>> connections;
	connections.resize(radios.size());

	// Read file for connections
	while(!file.eof()) {
		string line;
		getline(file, line);

		if(line.size() == 0) {
			continue;
		}

		stringstream ss(line);

		string udp_str, radio_str;
		ss >> udp_str;
		ss >> radio_str;

		BridgeConnection c;

		UdpUri cu;
		if(!UdpClient::Parse(udp_str, cu)) {
			return 1;
		}

		c.client = UdpClient::Ptr( new UdpClient(cu) );

		if(!Crazyradio::Parse(radio_str, c.config)) {
			return 1;
		}


		int ri = c.config.num;
		if(ri >= radios.size()) {
			cout << "No such radio #" << ri << endl;
		}

		if(connections[ri].size() > 0) {
			int chan = connections[ri][0].config.channel;

			// TODO: Also make sure they have the same rate
			if(chan != c.config.channel) {
				cout << "All configs on same radio must operate on same channel" << endl;
				return 1;
			}
		}

		connections[ri].push_back(c);
	}

	// Remove all unused radios
	for(int i = 0; i < radios.size(); i++) {
		if(connections[i].size() == 0) {
			radios.erase(radios.begin() + i);
			connections.erase(connections.begin() + i);
			i--;
		}
	}

	if(radios.size() == 0) {
		cout << "Nothing to do" << endl;
		return 1;
	}


	// Open all radios and udp clients
	for(int i = 0; i < radios.size(); i++) {

		if(radios[i]->open() != 0) {
			cout << "Failed to open radio #" << i << endl;
			return 1;
		}

		for(int j = 0; j < connections[i].size(); j++) {
			if(connections[i][j].client->open() != 0) {
				cout << "Failed to open client" << endl;
				return 1;
			}
		}

	}



	Bridge b(ctx, radios, connections);
	b.run();



	for(auto r : radios) {
		r->close();
	}

	libusb_exit(ctx);

	printf("Done!\n");
}

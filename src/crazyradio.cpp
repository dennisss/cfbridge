#include "crazyradio.h"

#include <stdio.h>
#include <stdbool.h>
#include <string.h>

#define CRAZYRADIO_PA_VID 0x1915
#define CRAZYRADIO_PA_PID 0x7777

#define TYPE_SETTER 0x40
#define TYPE_GETTER 0xC0

#define SET_RADIO_CHANNEL 0x01
#define SET_RADIO_ADDRESS 0x02
#define SET_DATA_RATE 0x03
#define SET_RADIO_POWER 0x04
#define SET_RADIO_ARD 0x05
#define SET_RADIO_ARC 0x06
#define ACK_ENABLE 0x10
#define SET_CONT_CARRIER 0x20
#define START_SCAN_CHANNELS 0x21
#define GET_SCAN_CHANNELS 0x21
#define LAUNCH_BOOTLOADER 0xFF


std::vector<Crazyradio::Ptr> Crazyradio::Enumerate(libusb_context *ctx) {

	libusb_device **list;

	std::vector<Crazyradio::Ptr> out;

	ssize_t s = libusb_get_device_list(ctx, &list);
	struct libusb_device_descriptor desc;

	for(unsigned i = 0; i < s; i++) {
		libusb_get_device_descriptor(list[i], &desc); // TODO: Error check this

		if(desc.idVendor == CRAZYRADIO_PA_VID && desc.idProduct == CRAZYRADIO_PA_PID) {
			out.push_back(std::shared_ptr<Crazyradio>(new Crazyradio(list[i])));
		}
	}

	libusb_free_device_list(list, 1);

	return out;
}


Crazyradio::Crazyradio(libusb_device *device) {
	this->device = device;
	libusb_ref_device(device);

}

Crazyradio::~Crazyradio() {
	libusb_unref_device(device);
}


int Crazyradio::open() {

	libusb_device_handle *h;
	if(libusb_open(device, &h) != 0) {
		printf("Failed to open radio\n");
		return 1;
	}

	return this->open(h);
}

int Crazyradio::open(libusb_device_handle *handle) {

	this->state = CFRADIO_STATE_IDLE;

	this->connected = 0;
	this->success_count = 0;
	this->fail_count = 0;
	this->handle = handle;

	if(libusb_set_configuration(this->handle, 1) != 0) {
		printf("Failed to set usb configuration\n");
		return 1;
	}

	struct libusb_config_descriptor config;

	libusb_get_descriptor(this->handle, LIBUSB_DT_CONFIG, 0, (unsigned char *) &config, sizeof(config));
	//printf("Has %d interfaces\n", config.bNumInterfaces);


	if(libusb_claim_interface(this->handle, 0) != 0) {
		printf("Couldn't claim interface\n");
		return 1;
	}


	// Configuring radio
	int res = 0;

	res |= set_radio_channel(80);
	res |= set_radio_address(0xE7E7E7E7E7);
	res |= set_data_rate(2);

	res |= set_cont_carrier(false);
	res |= set_radio_power(3); // 0dBm
	res |= set_radio_ard(32);
	res |= set_radio_arc(3);
	res |= set_ack_enable(true); // Set to false for single directional transfers


	if (res != 0) {
		printf("Configuration failed\n");
		return 1;
	}


	// Allocate a transfer for general use
	this->transfer = libusb_alloc_transfer(0);



	return 0;

}


int Crazyradio::close() {
	if(this->state != CFRADIO_STATE_IDLE) {
		libusb_cancel_transfer(this->transfer);
	}

	libusb_free_transfer(this->transfer);

	libusb_release_interface(this->handle, 0);

	libusb_close(this->handle);
	return 0;
}


int Crazyradio::notify() {

	if(this->state != CFRADIO_STATE_IDLE) {
		return 0;
	}

	RadioUri desiredConfig = activeConfig;
	int available = this->fetcher(&desiredConfig, &this->outbuf, this->arg);

	// Reconfiguring radio for new message if needed
	if(desiredConfig.addr != activeConfig.addr) {
		this->set_radio_address(desiredConfig.addr);
		activeConfig.addr = desiredConfig.addr;
	}

	/* if we can fetch a message send it, otherwise send a null packet */
	if(available) {
		//printf("send %d\n", radio->outbuf.size);
		submit_transfer(LIBUSB_ENDPOINT_OUT);
	}
	else {
		// TODO: Only do this if we are not in 1-directional communications mode
		submit_null();
	}

	return 0;
}

void Crazyradio::set_callbacks(CrazyradioFetcher fetcher, CrazyradioHandler handler, void *arg) {
	this->fetcher = fetcher;
	this->handler = handler;
	this->arg = arg;
}


void crazyradio_transfer_callback(struct libusb_transfer *transfer) {

	Crazyradio *radio = (Crazyradio *) transfer->user_data;

	switch(transfer->status){
		case LIBUSB_TRANSFER_COMPLETED:
			if(radio->state == CFRADIO_STATE_SENDING) {
				radio->submit_transfer(LIBUSB_ENDPOINT_IN);
				radio->state = CFRADIO_STATE_RECEIVING;
			}
			else if(radio->state == CFRADIO_STATE_RECEIVING) {
				radio->state = CFRADIO_STATE_IDLE;

				// Check status
				int ackReceived = radio->inbuf.status & 1;

				// Monitor connection status
				if(ackReceived) {
					radio->success_count++;
					radio->fail_count = 0;
					if(radio->success_count > 20 && !radio->connected){
						radio->connected = true;
						printf("Connected!\n");
					}
				}
				else {
					radio->success_count = 0;
					radio->fail_count++;
					if(radio->fail_count > 20 && radio->connected){
						radio->connected = false;
						printf("Lost connection!\n");
					}
				}


				if(transfer->actual_length > 1){
					crtp_message_t cmsg;
					cmsg.size = transfer->actual_length - 1;
					memcpy(&cmsg.header, radio->inbuf.data, cmsg.size);

					radio->handler(ackReceived, &radio->activeConfig, &cmsg, radio->arg);

					// We got non-empty packets, so trigger more to be sent
				}

				// Trigger a new transfer if needed

				//printf("usb done %d %d\n", transfer->actual_length, radio->inbuf.status & 1);
			}

			break;

		default:
			radio->state = CFRADIO_STATE_IDLE;
			printf("error in transfer: %s\n", libusb_error_name(transfer->status));
	}

	// Immediately after the last transaction was complete, send more data
	// TODO: In general we'd like to always fully utilize the
	radio->notify();
}



// LIBUSB_ENDPOINT_IN    LIBUSB_ENDPOINT_OUT
int Crazyradio::submit_transfer(int dir) {

	unsigned char *buffer;
	int size;

	if(dir == LIBUSB_ENDPOINT_OUT) {
		size = this->outbuf.size;
		buffer = (unsigned char *) &this->outbuf.header;
		this->state = CFRADIO_STATE_SENDING;
	}
	else if(dir == LIBUSB_ENDPOINT_IN) {
		size = 32 + 1; // MTU + 1 byte usb status header
		buffer = (unsigned char *) &this->inbuf;
		this->state = CFRADIO_STATE_RECEIVING;
	}

	libusb_fill_bulk_transfer(this->transfer, this->handle, 1 | dir, buffer, size,
		crazyradio_transfer_callback, this, 1000
	);

	// TODO: This can be error checked
	return libusb_submit_transfer(this->transfer);
}

// Used when there is no data that needs to be sent
int Crazyradio::submit_null() {
	this->outbuf.size = 1;
	this->outbuf.header = 0xff;
	return this->submit_transfer(LIBUSB_ENDPOINT_OUT);
}

int Crazyradio::set_radio_channel(uint8_t channel) {
	return libusb_control_transfer(
		handle, TYPE_SETTER, SET_RADIO_CHANNEL,
		channel, 0, NULL, 0, 0
	) < 0;
}

int Crazyradio::set_radio_address(uint64_t addr) {
	return libusb_control_transfer(
		handle, TYPE_SETTER, SET_RADIO_ADDRESS,
		0, 0, (unsigned char *) &addr, 5, 0
	) < 0;
}

int Crazyradio::set_data_rate(uint8_t rate) {
	return libusb_control_transfer(
		handle, TYPE_SETTER, SET_DATA_RATE,
		rate, 0, NULL, 0, 0
	) < 0;
}

int Crazyradio::set_radio_power(int level) {
	return libusb_control_transfer(
		handle, TYPE_SETTER, SET_RADIO_POWER,
		level, 0, NULL, 0, 0
	) < 0;
}

int Crazyradio::set_radio_ard(uint8_t length) {
	return libusb_control_transfer(
		handle, TYPE_SETTER, SET_RADIO_ARD,
		length | 0x80, 0, NULL, 0, 0
	) < 0;
}

int Crazyradio::set_radio_arc(uint8_t nretries) {
	return libusb_control_transfer(
		handle, TYPE_SETTER, SET_RADIO_ARC,
		nretries, 0, NULL, 0, 0
	) < 0;
}

int Crazyradio::set_ack_enable(bool enabled) {
	return libusb_control_transfer(
		handle, TYPE_SETTER, SET_RADIO_ARC,
		enabled? 1 : 0, 0, NULL, 0, 0
	) < 0;
}

int Crazyradio::set_cont_carrier(bool active) {
	return libusb_control_transfer(
		handle, TYPE_SETTER, SET_RADIO_ARC,
		active? 1 : 0, 0, NULL, 0, 0
	) < 0;
}

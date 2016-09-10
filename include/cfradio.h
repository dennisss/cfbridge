/*
	Crazyradio USB driver
	By itself it does not buffer messages in either direction.

	- To use it you need to specify a libusb context, a message handler, and a message emitter
	- Also, in the main loop of the program, libusb needs to be polled
*/

#include <stdint.h>
#include <libusb.h>

#include <crtp.h>


// TODO: Fetcher should also manage the active address
typedef int (*cfradio_fetcher)(crtp_message_t *buf);
typedef int (*cfradio_handler)(int status, crtp_message_t *msg);


typedef enum {
	CFRADIO_STATE_IDLE = 0,
	CFRADIO_STATE_SENDING,
	CFRADIO_STATE_RECEIVING
} cfradio_state;

typedef struct {
	uint8_t status;
	char data[32];
} usb_message;


typedef struct {
	libusb_device_handle *handle;
	cfradio_state state;
	uint64_t active_addr;

	struct libusb_transfer *transfer;
	usb_message inbuf;
	crtp_message_t outbuf;

	int connected;

	int success_count, fail_count;

	cfradio_fetcher fetcher;
	cfradio_handler handler;
} cfradio;


int cfradio_open(cfradio *radio, libusb_context *ctx, cfradio_fetcher fetcher, cfradio_handler handler);
int cfradio_close(cfradio *radio);

// Call when you want the radio to immediately start doing something
int cfradio_notify();

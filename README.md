CFBridge
========

A C based driver for communicating with a Crazyflie 2.0 using MAVLink

- Should be running PX4 : see http://dev.px4.io/hardware-crazyflie2.html
- MAVLink stream is accessible via local UDP ports (should be compatible with any existing GCS)

- Requires that libusb-1.0 is installed

Should run alongside a GCS with a Crazyradio PA. This is a commandline utility that configures the radio and allows routing Mavlink messages to a set of UDP ports for a set of radio addresses.


Building
--------

The PX4 Firmware source code should be in `../Firmware` relative to this README.

Run `make build`


Usage
-----

Connect a Crazyradio PA via USB and run `make run`.

- Once the CF2 is turned on, the bridge should echo 'Connected!'

- Then start up QGC listening on port 14550


Protocol
--------

Mavlink is sent to a UDP server hosted by this program. Those messages are packetized into CRTP messages and sent to the Crazyradio which forwards them to the Crazyflie. The reverse direction is similar

Transfers to the radio are prioritized over receiving data from the radio
When no data needs to be sent, the radio will 'idle' and will receive data at
a higher latency


Swarming
--------

TODO: Multiple Crazyflies can be multiplexed on one radio by changing the address
If multiple radios need to be handled, then multiple instances of this program
should be spawned


Future Optimizations
-------------

The Crazyradio PA uses a NRF24 chip which limits the MTU to 32 bytes (31 with CRTP header). Upgrading to an NRF51 usb radio and changing the syslink/crtp size here and in the STM and NRF onboard firmwares will allow ~250 byte packets, thus more efficient and not requiring the splitting of mavlink messages into chunks

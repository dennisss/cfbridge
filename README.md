CFBridge
========

Wireless MAVLink for Crazyflie 2.0's (CF2) running PX4. Should be plug and play with most GCS applications.


What this is
------------

1. A C based command line bridge for communicating with a Crazyflie 2.0 using MAVLink
	- The CF2 Should be running PX4 : see http://dev.px4.io/hardware-crazyflie2.html
	- MAVLink stream is accessible via local UDP ports (should be compatible with any existing GCS)

2. A driver for talking to Crazyradio PA's. (Note that the original crazyradios are not supported)
	- The driver is mostly separated from the bridge so should be useable in other applications as well


Building
--------

Make sure that you have the dev package for libusb-1.0 installed

Run `git submodule update --init` to sync the dependencies

Run `make build`


Usage
-----

Connect a Crazyradio PA via USB and run `make run`.

- Once the CF2 is turned on, the console should echo 'Connected!'

- Then start QGC listening on port 14550 (it listens on this port by default)


ROS
---

To connect to the crazyflie via mavros:

- Start up cfbridge using the above instructions

- Start mavros with a command such as `roslaunch mavros px4.launch fcu_url:="udp://:14550@127.0.0.1:14555"`
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

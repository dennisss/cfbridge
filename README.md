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
Make sure you have set the udev permissions to use the USB Radio. To do this, follow the steps listed [here](https://github.com/bitcraze/crazyflie-lib-python#setting-udev-permissions) and **restart** your computer.

Connect a Crazyradio PA via USB. 

Navigate to the cfbridge folder and run `make run`.

- Once the CF2 is turned on, the console should echo 'Connected!'

- Then start QGC listening on port 14550 (it listens on this port by default)
	- Note: The default parameters are equivalent running with a single line configuration of `udp://:14555@:14550 radio://0/80/2M/E7E7E7E7E7` (see below)


To configure these settings, make a file named `config.txt` and run `CONFIG=config.txt make run`.

Each line of the file is the configuration for a single Crazyflie. For example a file with the following two lines would connect to two Crazyflies on two addresses using a single radio.

	udp://:14556@:14551 radio://0/80/2M/0101010101
	udp://:14557@:14552 radio://0/80/2M/0202020202


Advanced (Swarming)
-------------------

Each Crazyflie is configured by a single line which is a pair of URIs:

- The client uri specifes how to forward/receive messages from other networked QGC's:
	- The format is `udp://[<local_addr>]:<local_port>@[<remote_addr>]:<remote_port>`
	- The local ip address/port are to configure which interface it should listen to for incoming messages from a GCS
	- The remote ip address/port are to specify where messages received from the radio is forwarded
	- If the `*_addr` part of either is ommited, `127.0.0.1` is assumed

- The radio uri specifes how to setup the radio
	- The format is `radio://<radio_number>/[<channel_number>]/[<data_rate>]/[<address>]`
		- All arguments after radio_number are optional
	- `radio_number` should be 0 to N-1 where N is the number of radios you have connected. The ordering is arbitrary based on how you connecting the radios
	- `channel_number` goes from 0 to 125 and occupies frequencies at 2.4GHz + (channel #) * 1MHz.
	- `data_rate` is either `250K`, `1M`, or `2M` representing the desired bitrate
	- `address` is a 10 character string representing the 5 byte address in hex format
		- Default address is `E7E7E7E7E7`
		- You should NOT pick `0000000001` etc. as these only have a few bit flips. Something like `0101010101` is preferred for transmission robustness.

Multiple Crazyflies can connect to one or more radios. We will assume that the # of radios <= # of Crazyflies, otherwise, it will not use all of the radios. The Crazyflies should be configured as follows:

1. Choose N different channel numbers where N is the number of radios
	-  In general you should not pick consecutive channel numbers if possible and you should try not to overlap any WiFi networks in the area.

2. Assign a channel number to each Crazyflie ideally with not too many on a single channel.

3. Using QGroundControl, change the Syslink parameters to match this channel and give each Crazyflie a unique address

4. `make run config.txt` with the appropriate lines in the config file
	- Note: Each radio should have a single unique channel


Performance Note: This whole program runs reactively as a single thread. Should your USBs be connected to separate buses, you may get slightly more performance by starting multiple instances of this bridge for subsets of the radios


TODO: We should also allow configuring the idle rate: When no messages are being transfered, this is how often it will send messages in bidirectional mode to get data back

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


Future Optimizations
-------------

The Crazyradio PA uses a NRF24 chip which limits the MTU to 32 bytes (31 with CRTP header). Upgrading to an NRF51 usb radio and changing the syslink/crtp size here and in the STM and NRF onboard firmwares will allow ~250 byte packets, thus more efficient and not requiring the splitting of mavlink messages into chunks

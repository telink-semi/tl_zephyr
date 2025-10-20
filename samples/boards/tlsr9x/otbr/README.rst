Telink Thread border router
###########################
A Thread Border Router connects a Thread Network to other IPbased networks, such as Wi-Fi or Ethernet.
A Thread Network requires a Border Router to connect to other networks.
The Border Router provides services for devices within the Thread Network,
including routing services for off-network operations, bidirectional connectivity over IPv6 infrastructure links,
and service registry to enable DNS-based service discovery.

Main diagram
************
Telink Thread border router consists of two boards: tlsr9118bdk40d as main router and tlsr9528a as RCP.

Wi-Fi network <-*Wi-Fi*-> **tlsr9118bdk40d** <-*UART Spinel*-> **tlsr9528a** <-*Thread*-> Thread network

Building RCP FW (*tlsr9528a*)
*****************************
Under Zephyr environment build and flash RCP

.. code-block:: bash

	rm -rf build_ot_rcp
	west build -b tlsr9528a -d build_ot_rcp  samples/net/openthread/coprocessor -- -DOVERLAY_CONFIG=overlay-rcp.conf
	west flash --erase -d build_ot_rcp


Building Main OTBR FW (*tlsr9118bdk40d*)
****************************************
Under Zephyr environment build and flash as usual (no any special options).
Select tlsr9118bdk40d_v1 or tlsr9118bdk40d depending on your hardware.

.. code-block:: bash

	rm -rf build_otbr
	west build -b tlsr9118bdk40d_v1 -d build_otbr samples/boards/tlsr9x/otbr
	west flash --erase --update-n22 -d build_otbr

Default boards connections
**************************
To establish main router and RCP connection their UARTs interconnection is required.
The pinout is defined in the next overlays:

* *samples/boards/tlsr9x/otbr/boards/tlsr9118bdk40d.overlay*
* *samples/boards/tlsr9x/otbr/boards/tlsr9118bdk40d_v1.overlay*
* *samples/net/openthread/coprocessor/boards/tlsr9528a.overlay*

Which without modification corresponds to

=====================  =====================
*tlsr9118bdk40d*       *tlsr9528a*
=====================  =====================
**tx** p16 (J20 16)    **rx** pc7 (J5 4)
**rx** p15 (J20 18)    **tx** pc6 (J5 2)
**rts** p18 (J20 12)   **cts** pc4 (J3 19)
**cts** p17 (J20 14)   **rts** pc5 (J3 21)
**gnd** (J20 35)       **gnd** (J3 30)
=====================  =====================

Configure Wi-Fi and Thread credentials
**************************************
File *samples/boards/tlsr9x/otbr/prj.conf* contains credentials.
Set the next configs according to your Wi-Fi network and required Thread network:

* CONFIG_TELINK_OTBR_WIFI_SSID
* CONFIG_TELINK_OTBR_WIFI_PASSWORD
* CONFIG_OPENTHREAD_CHANNEL
* CONFIG_OPENTHREAD_PANID
* CONFIG_OPENTHREAD_XPANID
* CONFIG_OPENTHREAD_NETWORKKEY
* CONFIG_OPENTHREAD_NETWORK_NAME

Using border router
*******************
Power on tlsr9528a and tlsr9118bdk40d boards. Use tlsr9118bdk40d console (Flashing UART).
And wait boot process: connecting Wi-Fi, creating Thread network...

.. code-block:: console

	ot omr addr: fd6a:c41a:dced:1:ce15:cf04:eecf:9b53
	ot omr net : fd6a:c41a:dced:1::/64
	ot active dataset: 0e080000000000010000000300001235060004001fffe00208ff0db800000000000708fde770d7092819070510ff112233445566778899aabbccddeeff030974656c696e6b2d6f740102141804108a6e1d9875742d29ab11523b4f02bdcf0c0402a0f7f8

which means that Wi-Fi network is connected, Thread network created and OMR preffix is assigned.
For now Thread devices can be accessible from LAN using addressees which belongs to OMR mask.
For joining new Thread devices OT network dataset is printed.

Now it's time to join thread device. USB NRF dongle (nRF52840) with flashed ot-cli can be use for testing. From its console:

.. code-block:: console

	ot factoryreset
	ot dataset set active 0e080000000000010000000300001235060004001fffe00208ff0db800000000000708fde770d7092819070510ff112233445566778899aabbccddeeff030974656c696e6b2d6f740102141804108a6e1d9875742d29ab11523b4f02bdcf0c0402a0f7f8
	Done
	ot ifconfig up
	Done
	ot thread start
	Done
	ot ipaddr
	fd6a:c41a:dced:1:d567:5def:2c66:be64
	fdbb:db68:ab3:8644:0:ff:fe00:7c01
	fdbb:db68:ab3:8644:278e:b78d:99b3:f587
	fe80:0:0:0:7c89:5317:c524:d333
	Done

Here OMR address can be observed *fd97:a95e:5716:1:d567:5def:2c66:be64* which corresponds to OMR mask *fd97:a95e:5716:1::/64*.

Now from PC (same LAN as BR) check IPv6 address:

.. code-block:: bash

	ip -6 addr
	...
	3: wlo1: <BROADCAST,MULTICAST,UP,LOWER_UP> mtu 1500 state UP qlen 1000
	    inet6 fd54:e265:e92c::1001/128 scope global dynamic noprefixroute
	...

Ping PC from Thread device (USB NRF console):

.. code-block:: console

	ot ping fd54:e265:e92c::1001
	16 bytes from
	fd54:e265:e92c:0:0:0:0:1001
	: icmp_seq=1 hlim=64 time=63ms
	packets transmitted, 1 packets received.
	Packet loss = 0.0%.
	Round-trip min/avg/max = 63/63.0/63 ms.
	Done

Ping Thread device from PC:

.. code-block:: bash

	ping fd6a:c41a:dced:1:d567:5def:2c66:be64
	PING fd6a:c41a:dced:1:d567:5def:2c66:be64(fd6a:c41a:dced:1:d567:5def:2c66:be64) 56 data bytes
	64 bytes from fd6a:c41a:dced:1:d567:5def:2c66:be64: icmp_seq=1 ttl=64 time=45.3 ms
	64 bytes from fd6a:c41a:dced:1:d567:5def:2c66:be64: icmp_seq=2 ttl=64 time=28.6 ms
	64 bytes from fd6a:c41a:dced:1:d567:5def:2c66:be64: icmp_seq=3 ttl=64 time=28.5 ms
	...

Thread border router shell for controlling and retrieving its status is available over telnet.
To connect from PC:

.. code-block:: console

	avahi-resolve -n telink-otbr.local
	telink-otbr.local	fdef:e3be:8d3c:0:66f9:47ff:fef0:120

	telnet fdef:e3be:8d3c:0:66f9:47ff:fef0:120
	Trying fdef:e3be:8d3c:0:66f9:47ff:fef0:120...
	Connected to fdef:e3be:8d3c:0:66f9:47ff:fef0:120.
	Escape character is '^]'.

Type "help" to see available commands.

For sure it's possible access services directly by host name:

.. code-block:: console

	telnet telink-otbr.local

But most distros do not support IPv6 name resolution out of the box. To fix this check your
*/etc/nsswitch.conf* file. Usually you have the next resolution:

.. code-block:: console

	 hosts:          files mdns4_minimal [NOTFOUND=return] dns

As you see only IPv4 name resolution is available. So change it:

.. code-block:: console

	 hosts:          files mdns_minimal [NOTFOUND=return] dns

Commissioning Matter devices
****************************

To proceed commissioning procedure with Matter device you need border router dataset.
This dataset as was mentioned is available in console after boot:

.. code-block:: console

	ot active dataset: 0e080000000000010000000300001235060004001fffe00208ff0db800000000000708fde770d7092819070510ff112233445566778899aabbccddeeff030974656c696e6b2d6f740102141804108a6e1d9875742d29ab11523b4f02bdcf0c0402a0f7f8

In case if console is not available it can be obtained using telnet:

.. code-block:: console

	telnet telink-otbr.local
	Trying fd11:1111:1122:2222:66f9:47ff:fef0:120%3...
	Connected to telink-otbr.local.
	Escape character is '^]'.
	ot dataset active -x

	0e080000000000010000000300001235060004001fffe00208ff0db800000000000708fde770d7092819070510ff112233445566778899aabbccddeeff030974656c696e6b2d6f740102141804108a6e1d9875742d29ab11523b4f02bdcf0c0402a0f7f8
	Done

Now just run chip-tool as usual (Matter device should be in BLE advertisement mode):

.. code-block:: console

	rm -rf /tmp/chip_*
	./chip-tool pairing ble-thread 1234 hex:0e080000000000010000000300001235060004001fffe00208ff0db800000000000708fde770d7092819070510ff112233445566778899aabbccddeeff030974656c696e6b2d6f740102141804108a6e1d9875742d29ab11523b4f02bdcf0c0402a0f7f8 20202021 3840

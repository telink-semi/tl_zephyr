.. _tpll_prx:

TPLL: Receiver
#############################

.. contents::
   :local:
   :depth: 2

The sample shows how to use the TPLL protocol in receiver mode on a Telink TL322X device.
It shows how to configure the 2.4G radio (TPLL) to receive packets through the dual-core shared-memory command interface.

Requirements
************

The sample is built for the Telink TL322X series development kit (for example ``tl3228x/telink_tl322x``).

Additionally, if you want to test the TPLL Transmitter functionality, you need to build and run the :ref:`tpll_ptx` sample.
You can use any two TL322X development kits.

Overview
********

The sample runs on the TL322X host core and controls the 2.4G radio on the N22 core through shared-memory commands (d25f).
The radio is configured in receiver mode with the following settings:

* Mode: ``TPLL_MODE_PRX``
* Bitrate: ``TPLL_BITRATE_2MBPS``
* Access code length: 5 bytes (``ADDRESS_WIDTH_5BYTES``)
* Base addresses and pipe prefixes for all 8 pipes
* RF channel: 14
* All pipes enabled (``0xff``)

After the configuration is applied, the Receiver starts listening for packets.
When a packet is received, the N22 core notifies the host core, and the received payload is printed to the console in hex format.

User interface
***************

The sample prints each received packet to the console in the following format:

.. code-block:: console

   RX data len=<length>: <hex bytes>

Configuration
*************

The following configuration options are set in :file:`prj.conf`:

* ``CONFIG_BT`` - enables the Bluetooth subsystem.
* ``CONFIG_BT_TLX`` - enables the Telink TLX Bluetooth controller.
* ``CONFIG_SOC_RISCV_TELINK_TL322X`` - selects the TL322X SoC.
* ``CONFIG_TELINK_TL322X_ENABLE_N22`` - enables the N22 core that runs the 2.4G radio firmware.

Building and running
********************

The Receiver sample can be found under :file:`samples/tpll/tpll_prx`.

Build the sample for the TL322X board:

.. code-block:: console

   west build -b tl3228x/telink_tl322x samples/tpll/tpll_prx

See :ref:`building` for information about how to build the application and :ref:`programming` for how to program it.

.. _tpll_prx_testing:

Testing
=======

To test the Receiver functionality, you need a Transmitter sample on another development kit:

1. Build and program the :ref:`tpll_ptx` sample on one TL322X development kit.
#. Build and program the Receiver sample on another TL322X development kit.
#. Power on both kits.
#. Optionally, connect to the Receiver kit with a terminal emulator.
#. Observe that every packet sent by the Transmitter is printed by the Receiver as ``RX data len=<length>: <hex bytes>``.

Dependencies
************

The sample uses the following Zephyr libraries:

* :ref:`zephyr:kernel_api`:

  * :file:`include/kernel.h`
  * :file:`include/irq.h`

* :ref:`zephyr:logging_api`

In addition, it uses the following Telink modules:

* :file:`tl_common.h`
* :file:`tlx_bt.h`
* The 2.4G dual-core shared-memory service (:file:`stack/multicore_comm/service/service_d25f.h`)
* The TPLL 2.4G stack (:file:`stack/2p4g/tl_tpll/tl_tpll.h`)
.. _tpll_ptx:

TPLL: Transmitter
################################

.. contents::
   :local:
   :depth: 2

The sample shows how to use the TPLL protocol in transmitter mode on a Telink TL322X device.       
It shows how to configure the 2.4G radio (TPLL) to transmit packets through the dual-core shared-memory command interface.

Requirements
************

The sample is built for the Telink TL322X series development kit (for example ``tl3228x/telink_tl322x``).

Additionally, if you want to test the TPLL Receiver functionality, you need to build and run the :ref:`tpll_prx` sample.
You can use any two TL322X development kits.

Overview
********

The sample runs on the TL322X host core and controls the 2.4G radio on the N22 core through shared-memory commands (d25f).
The radio is configured in transmitter mode with the following settings:

* Mode: ``TPLL_MODE_PTX``
* Bitrate: ``TPLL_BITRATE_2MBPS``
* Access code length: 5 bytes (``ADDRESS_WIDTH_5BYTES``)
* Base addresses and pipe prefixes for all 8 pipes
* RF channel: 14
* All pipes enabled (``0xff``)

The Transmitter periodically starts a transmission, waits for the TX completion event from the N22 core, sleeps for 100 milliseconds, and then transmits again.
The two samples must use the same RF settings (access codes, prefixes, bitrate and channel) to communicate with each other.

User interface
***************

The sample does not expose a user interface.
Successful transmission is indicated by the TX completion event returned by the N22 core; the loop counter is incremented after each completed transmission.

Configuration
*************

The following configuration options are set in :file:`prj.conf`:

* ``CONFIG_BT`` - enables the Bluetooth subsystem.
* ``CONFIG_BT_TLX`` - enables the Telink TLX Bluetooth controller.
* ``CONFIG_SOC_RISCV_TELINK_TL322X`` - selects the TL322X SoC.
* ``CONFIG_TELINK_TL322X_ENABLE_N22`` - enables the N22 core that runs the 2.4G radio firmware.

Building and running
********************

The Transmitter sample can be found under :file:`samples/tpll/tpll_ptx`.

Build the sample for the TL322X board:

.. code-block:: console

   west build -b tl3228x/telink_tl322x samples/tpll/tpll_ptx

See :ref:`building` for information about how to build the application and :ref:`programming` for how to program it.

.. _tpll_ptx_testing:

Testing
=======

To test the Transmitter functionality, you need a Receiver sample on another development kit:

1. Build and program the Transmitter sample on one TL322X development kit.
#. Build and program the :ref:`tpll_prx` sample on another TL322X development kit.
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
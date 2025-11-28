.. zephyr:code-sample:: nfc
   :name: NFC

   Read and write NFC data using the NFC subsys API.

Overview
********

This example demonstrates how to use the Zephyr NFC subsystem to emulate an NFC Tag (Type 5 Tag - ISO15693)
that stores a multi-language NDEF text message. The implementation is designed to work with the **ST25DV**
family of NFC dynamic tags.

NOTE: Currently this sample is broken due to NFC subsys not present.
This is for future improvements.

Three NDEF Text Records are created and encoded into one message, containing the phrase:

- "Hello World!" in English
- "Hallo Verden!" in Norwegian
- "Witaj świecie!" in Polish

When an NFC-enabled device (such as a smartphone) scans the tag, it reads these text messages.

The code also demonstrates how to:

- Initialize an NFC Tag device (ST25DVxxKC)
- Configure the tag type
- Encode NDEF text records
- Set and start NFC Tag emulation

Building and Running
********************

This sample can be found under :zephyr_file:`samples/subsys/nfc` in the Zephyr tree.

**Build for the desired board** (example for TLSR9518ADK80D):

.. code-block:: bash

   west build -b tlsr9518adk80d -d build samples/subsys/nfc -- -DOVERLAY_CONFIG=boards/st25dv.conf -DDTC_OVERLAY_FILE=boards/tlsr9518adk80d.overlay

Expected Output
===============

When the device boots, you should see console output similar to:

::

  Starting NFC Text Record example
  Device = ok: st25dvxxkc
  NFC configuration done (0)

Once configured, bring an NFC-enabled phone close to the board. The phone should detect an NFC Tag and display:

::

  Hello World!
  Hallo Verden!
  Witaj świecie!

License
=======

This sample is distributed under the Apache 2.0 license.

- Original work (c) 2023 Sendrato
- Modifications (c) 2025 Telink Semiconductor

SPDX-License-Identifier: Apache-2.0

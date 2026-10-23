Dynamic NFC/RFID tag IC with EEPROM and fast transfer mode capability demo
##########################################################################

The demo application shows functionality of dynamic *NFC/RFID*.
These parts are ICs with 4-, 16-, or 64-Kbit *EEPROM* and have fast transfer mode capability.
Supported ICs are: *ST25DV04KC*, *ST25DV16KC*, *ST25DV64KC*.
Demo covers two possible ST25DV modes mailbox data exchange and NFC tag.

Building and flashing (*tl7218x*)
*********************************

.. code-block:: bash

	rm -rf build_*
	west build -b tl7218x -d build_nfc samples/boards/tlsr9x/st25dv-nfc-tap
	west flash --erase -d build_nfc

HW connection
*************

For details refer to generated .dts file *build_nfc/zephyr/zephyr.dts*

In case of *tl7218x*

.. code-block:: dts

	st25dv_matter: st25dv {
	    compatible = "st25dv";
	    i2c-bus = < &i2c >;
	    isr-port = < &gpioe >;
	    isr-port-pin = < 0x5 >;
	};

	i2c: i2c@80140280 {
	    compatible = "telink,tlx-i2c";
	    reg = < 0x80140280 0x40 >;
	    #address-cells = < 0x1 >;
	    #size-cells = < 0x0 >;
	    status = "okay";
	    clock-frequency = < 0x61a80 >;
	    pinctrl-0 = < &i2c_scl_pe6_default &i2c_sda_pe7_default >;
	    pinctrl-names = "default";
	    phandle = < 0x17 >;
	};

So we have:

- *ISR GPO, pin 7 for SO8* - PE5
- *I2C SCL, pin 6 for SO8* - PE6
- *I2C SDA, pin 5 for SO8* - PE7

Don't forget to also connect ground and power supply 3.3V

NFC data exchange demo
**********************

After application startup console output is the next:

.. code-block:: console

	D: st25dv st25dv_init_workq
	*** Booting Zephyr OS build tl_v1.4.0-v4.1.0-27-g3ca84e53afa5 ***
	I: application started
	D: st25dv st25dv_init
	D: st25dv st25dv_probe_internal
	I: st25dv chip id 51 rev 11
	D: st25dv already unlocked
	D: st25dv st25dv_update_registers
	D: st25dv gpo 1 mode already set
	D: st25dv gpo 2 mode already set
	D: st25dv mailbox mode already enabled
	D: st25dv radio state already set
	D: st25dv st25dv_reset
	D: st25dv init ok
	D: st25dv st25dv_mode_set
	D: st25dv st25dv_mode_set_internal
	D: st25dv mode mailbox
	D: st25dv st25dv_set_callback
	D: st25dv st25dv_mode_get
	D: st25dv st25dv_mode_get_internal
	I: nfc device mode mailbox
	I: nfc device mailbox data loopback...
	the next one byte data is reserved for commands
	0x00 - finish the application (no future commands possible)
	0x01 - write tag eeprom
	0x02 - erase tag eeprom
	0x03 - switch to tag mode (no future commands possible)
	0x04 - switch off (no future commands possible)
	commands are not sent back


On phone install `ST25 NFC Tap <https://play.google.com/store/apps/details?id=com.st.st25nfc>`_ application.

1. Start *ST25 NFC Tap* application and approach phone to tag.
2. From main application menu select "Send custom command".
3. Write data to mailbox (under *DATA* field first byte is data length minus one next bytes are actually data)

	.. image:: img/wr_msg.jpg

	In console:

	.. code-block:: console

		D: st25dv data received
		D: st25dv
		D: 11 22                   |."
		I: nfc device rx
		I: 11 22                   |."
		D: st25dv st25dv_mailbox_send
		D: st25dv mailbox send ok
		I: nfc device rx data written back into mailbox

4. Get length of income data in mailbox (in *tag response* second byte is income data length minis one)

	.. image:: img/rd_msg_len.jpg

5. Read data from mailbox (under *DATA* put *tag response* of previous step)

	.. image:: img/rd_msg.jpg

	In console:

	.. code-block:: console

		D: st25dv data sent
		I: nfc device tx

Specific mailbox commands
*************************

1. 0x00 - finish the application, no future commands possible so reset is required.

	In console:

	.. code-block:: console

		D: st25dv data received
		D: st25dv
		D: 00                      |.
		I: nfc device rx
		I: 00                      |.
		I: application exit
		I: 00                      |.
		D: st25dv st25dv_deinit
		D: st25dv st25dv_reset
		D: st25dv deinit ok
		I: application finished

	Tag is no longer active.

2. 0x01 - write tag to eeprom

	In console:

	.. code-block:: console

		D: st25dv data received
		D: st25dv
		D: 01                      |.
		I: nfc device rx
		I: 01                      |.
		I: nfc device write tag
		I: 01                      |.
		D: st25dv st25dv_tag_type5_add
		D: st25dv st25dv_type5_tag_precondition
		I: st25dv memory size 8192 bytes
		I: st25dv tag size 25 bytes
		D: st25dv st25dv_mode_get_internal
		D: st25dv st25dv_mode_set_internal
		D: st25dv mode tag
		D: st25dv st25dv_mode_set_internal
		D: st25dv mode mailbox
		D: st25dv tag wtite ok
		I: nfc device tag data set

	Memory can be read by application to check the result.

3. 0x02 - erase tag from eeprom

	In console:

	.. code-block:: console

		D: st25dv data received
		D: st25dv
		D: 02                      |.
		I: nfc device rx
		I: 02                      |.
		I: nfc device erase tag
		I: 02                      |.
		D: st25dv st25dv_tag_type5_rm
		D: st25dv st25dv_type5_tag_precondition
		I: st25dv memory size 8192 bytes
		I: st25dv tag size 25 bytes
		D: st25dv st25dv_mode_get_internal
		D: st25dv st25dv_mode_set_internal
		D: st25dv mode tag
		D: st25dv st25dv_mode_set_internal
		D: st25dv mode mailbox
		D: st25dv tag erase ok
		I: nfc device tag data erased

	Memory can be read by application to check the result.

4. 0x03 - switch to tag mode, no future commands possible so reset is required.

	In console:

	.. code-block:: console

		D: st25dv data received
		D: st25dv
		D: 03                      |.
		I: nfc device rx
		I: 03                      |.
		I: nfc device switch to tag mode
		I: 03                      |.
		D: st25dv st25dv_mode_set
		D: st25dv st25dv_mode_set_internal
		D: st25dv mode tag
		D: st25dv st25dv_mode_get
		D: st25dv st25dv_mode_get_internal
		I: nfc device mode nfc tag

	Now approach android phone close to tag.
	When phone requested "Choose an application" select "Tags New tag collected".
	And you should see tag data: *Hello World!*

5. 0x04 - switch off, no future commands possible so reset is required.

	In console:

	.. code-block:: console

		D: st25dv data received
		D: st25dv
		D: 04                      |.
		I: nfc device rx
		I: 04                      |.
		I: nfc device switch off
		I: 04                      |.
		D: st25dv st25dv_mode_set
		D: st25dv st25dv_mode_set_internal
		D: st25dv mode off
		D: st25dv st25dv_mode_get
		D: st25dv st25dv_mode_get_internal
		I: nfc device mode switched off
		D: st25dv rf field falling
		I: nfc device field off

	Tag is no longer active.

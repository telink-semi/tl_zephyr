BLE Peripheral with Low Battery Detection
#########################################

Overview
********

This sample combines a minimal BLE peripheral application (connectable
advertising plus the Device Information Service) with a periodic battery
voltage check, mirroring the low battery detection flow of the Telink BLE
SDK ``acl_peripheral_demo`` application (``app_battery_power_check()``).

The battery voltage (VBAT) is measured through the ADC internal VBAT
channel, configured via the devicetree ``zephyr,user`` node in the per-board
overlays, following the same pattern as the official ``adc_dt`` sample.

When the measured voltage drops below the alarm threshold
(:kconfig:option:`CONFIG_APP_BATT_ALARM_MV`):

1. The red LED blinks three times as the alarm indication.
2. The on-board button *key_1* is armed as the deep sleep wakeup source
   (a low level on the pad wakes the MCU).
3. The application enters deep sleep. Only pressing *key_1* wakes the MCU;
   waking from deep sleep reboots the application.

A low battery flag is kept in the PM power-on-clear analog register
(``PM_ANA_REG_POWER_ON_CLR_BUF1``, preserved across deep sleep) to implement
a 200 mV recovery hysteresis: once the alarm has fired, the battery is
considered recovered only when the voltage rises above *alarm threshold +
200 mV*.

Requirements
************

- One of the supported Telink boards: ``tl5218x``, ``tl5218x_retention``,
  ``tl3238x`` or ``tl3238x_retention``
- UART console for log output

Building and Flashing
*********************

.. code-block:: console

	rm -rf build_*
	west build -b tl5218x -d build_lbd samples/boards/tlsr9x/ble_peripheral_low_battery_detect
	west flash --erase -d build_lbd

For *tl3238x*, replace the board name accordingly (``-b tl3238x``).

Configuration Options
*********************

.. list-table::
   :header-rows: 1
   :widths: 30 20 50

   * - Option
     - Default
     - Description
   * - ``CONFIG_BATT_CHECK_ENABLE``
     - y
     - Enable the battery power check module (selects ``PM``)
   * - ``CONFIG_APP_BATT_ALARM_MV``
     - 2000
     - Low battery alarm threshold in mV
   * - ``CONFIG_APP_BATT_CHECK_INTERVAL_MS``
     - 500
     - Battery check interval in ms. The sample main loop currently polls
       once per second.
   * - ``CONFIG_BATT_RAW_IS_MV``
     - y on TL521X / TL323X, n otherwise
     - The TL521X and TL323X ADC drivers report the measured voltage in mV
       directly in the sample buffer. The other Telink ADC drivers return
       raw codes, which are converted with ``adc_raw_to_millivolts_dt()``.

Devicetree Configuration
************************

The per-board overlays (``boards/<board>.overlay``) define the ``zephyr,user``
node with one ADC io-channel bound to the internal VBAT input, and disable
the peripherals not used by this sample:

.. code-block:: dts

	zephyr,user {
	    io-channels = <&adc 0>;
	};

	&adc {
	    channel@0 {
	        reg = <0>;
	        zephyr,gain = "ADC_GAIN_1";
	        zephyr,reference = "ADC_REF_INTERNAL";
	        zephyr,acquisition-time = <ADC_ACQ_TIME_DEFAULT>;
	        zephyr,resolution = <24>;
	        zephyr,input-positive = <DT_ADC_VBAT>;
	    };
	};

HW Connections
**************

Only on-board resources are used, no external wiring is required:

- *VBAT ADC input* - internal VBAT channel (no external divider)
- *Alarm LED* - ``led_red`` (PA3 on *tl5218x*, PC2 on *tl3238x*, active high)
- *Deep sleep wakeup button* - ``key_1`` (PC2 on *tl5218x*, PC4 on
  *tl3238x*, ``GPIO_ACTIVE_LOW | GPIO_PULL_UP``): the button's internal
  pull-up keeps the pad high while released, so pressing the button asserts
  the low level that wakes the MCU from deep sleep

Low Battery Alarm Behavior
**************************

.. code-block:: console

	VBAT: 1898 mV
	sleep <wakeup-port> <pin> raw=1

1. The measured VBAT is at or below the alarm threshold (or below
   *threshold + 200 mV* while the low battery flag is set).
2. The red LED blinks three times.
3. The low battery flag is stored in ``PM_ANA_REG_POWER_ON_CLR_BUF1``.
4. *key_1* is armed as the deep sleep wakeup pad
   (``cpu_set_gpio_wakeup(..., WAKEUP_LEVEL_LOW, ...)``).
5. Buffered log messages are flushed over the UART (deferred logging mode).
6. The MCU enters deep sleep (``cpu_sleep_wakeup_32k_rc(DEEPSLEEP_MODE,
   PM_WAKEUP_PAD, 0)``), which is only exited by pressing *key_1*. Waking
   from deep sleep reboots the application.

While the voltage is above the threshold, the application keeps advertising
and prints the measured voltage once per second.

Console Output
**************

Normal operation:

.. code-block:: console

	*** Booting Zephyr OS build v4.1.99-... ***
	boot <wakeup-port> <pin> raw=1
	VBAT: 3312 mV
	Bluetooth initialized
	Advertising successfully started
	VBAT: 3312 mV
	...

The ``boot ... raw=`` and ``sleep ... raw=`` lines are pad diagnostics: with
the button released, the pad must read 1 (pull-up). A 0 reading means the
pad is pulled low externally (e.g. a stuck button) and would wake the MCU
immediately after deep sleep entry.

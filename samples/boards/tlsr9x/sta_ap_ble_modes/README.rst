The sample can be built in one of four idle power measurement modes using
an extra configuration file:

* ``overlay-sta.conf``: STA-only. The sample stays disconnected for 30 seconds,
  then connects and remains idle so both disconnected and connected baselines
  can be captured in one trace.
* ``overlay-ap.conf``: SoftAP-only, with no connected clients expected.
* ``overlay-apsta.conf``: concurrent SoftAP + STA, with no AP clients or active
  application traffic expected.
* ``overlay-apsta-ble.conf``: concurrent SoftAP + STA plus BLE advertising.

For example:
   west build -b tlsr9118bdk40d zephyr/samples/boards/tlsr9x/sta_ap_ble_modes -- -DEXTRA_CONF_FILE=overlay-apsta-ble.conf


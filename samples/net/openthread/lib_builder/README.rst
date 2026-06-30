Telink openthread library builder
#################################
This is a project to create Telink openthread prebuilt libraries.
This project is used by github CI, see `openthread_telink_lib <../../../../.github/workflows/telink-ot-lib.yaml>`_ for details.
This job is creating Telink openthread libraries set as its artifacts, which are are usually populated in `official Telink openthread libraries repository <https://github.com/telink-semi/openthread_telink_lib>`_.


CI libraries generating
***********************
CI job provides fully automated generating Telink openthread prebuilt librarie using current Zephyr state: Zephyr sources and openthread module sources.
Libraries configurations are located at `overlays/ <overlays/>`_ directory.
Overlays can be edited or extended, CI job automatically iterates over all available overlays.
As building time *CONFIG_TELINK_OT_LIB_BUILD_TIMESTAMP* is used and can be modified in a usual Zephyr way.

CI job doing next:

- Compiling libraries (multiple compilers are supported)
- Installing headers
- Making Zephyr module (generating module.yml, CMakelists.txt and Kconfig)
- Expiring libraries configurations to use in Zephyr (to link again rebuilt library instead openthread sources)
- Generating readme with main build information

Manual building library
***********************
Library build can be triggered in a usual zephyr way additionally providing required openthread overlay by setting variable *OVERLAY_CONFIG* with required library configuration.
See `overlays/ <overlays/>`_ directory for available library types.

.. code-block:: console

    west build -b tl7218x -d build_cli samples/net/openthread/lib_builder/ -- -DOVERLAY_CONFIG=overlays/mtd.conf

Additionally special target is created to install openthread library headers.


.. code-block:: console

    west build -b tl7218x -d build_ot_lib samples/net/openthread/lib_builder/ -t ot-headers

Building details
****************

- *deterministic build* (each build triggering is producing same output for same sources).
  Important controlled part of this process is a build timestamp *CONFIG_TELINK_OT_LIB_BUILD_TIMESTAMP*.
  This timestamp is used by compiler expand date/time depending macros. This config can be changed in usual Zephyr way.
  To obtain timestamp from date/time as example next service can be used `https://www.unixtimestamp.com/ <https://www.unixtimestamp.com/>`_
  To have same cross PC building take timezone into account (Github CI usually using UTC).

- *library overlays.* 
  Libraries overlays are located at *overlays/* project directory and can be edited to have desired openthread configuration.
  Additionally new overlay (.conf file which is usual Zepher kernel config file with openthread related options) can be provided to extend libraries set.

Link application against prebuilt library
*****************************************
To link application against prebuilt library provide variable *OVERLAY_CONFIG* with corresponding library type.
See `configs/ <../../../../../modules/lib/openthread_telink_lib/configs/>`_ directory under `openthread_telink_lib module <../../../../../modules/lib/openthread_telink_lib>`_ for available configurations.

As example building *openthread-cli*:

.. code-block:: console

    west build -b tl7218x -d build_cli samples/net/openthread/cli/ -- -DOVERLAY_CONFIG=$(west topdir)/modules/lib openthread_telink_lib/configs/libopenthread-mtd.conf

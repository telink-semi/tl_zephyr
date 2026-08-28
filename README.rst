.. raw:: html

   <a href="https://www.zephyrproject.org">
     <p align="center">
       <picture>
         <source media="(prefers-color-scheme: dark)" srcset="doc/_static/images/logo-readme-dark.svg">
         <source media="(prefers-color-scheme: light)" srcset="doc/_static/images/logo-readme-light.svg">
         <img src="doc/_static/images/logo-readme-light.svg">
       </picture>
     </p>
   </a>

   <a href="https://bestpractices.coreinfrastructure.org/projects/74"><img
   src="https://bestpractices.coreinfrastructure.org/projects/74/badge"></a>
   <a
   href="https://github.com/zephyrproject-rtos/zephyr/actions/workflows/twister.yaml?query=branch%3Amain">
   <img
   src="https://github.com/zephyrproject-rtos/zephyr/actions/workflows/twister.yaml/badge.svg?event=push"></a>


The Zephyr Project is a scalable real-time operating system (RTOS) supporting
multiple hardware architectures, optimized for resource constrained devices,
and built with security in mind.

The Zephyr OS is based on a small-footprint kernel designed for use on
resource-constrained systems: from simple embedded environmental sensors and
LED wearables to sophisticated smart watches and IoT wireless gateways.

The Zephyr kernel supports multiple architectures, including ARM (Cortex-A,
Cortex-R, Cortex-M), Intel x86, ARC, Nios II, Tensilica Xtensa, and RISC-V,
SPARC, MIPS, and a large number of `supported boards`_.

.. below included in doc/introduction/introduction.rst


Getting Started
***************

Welcome to Zephyr! See the `Introduction to Zephyr`_ for a high-level overview,
and the documentation's `Getting Started Guide`_ to start developing.

Telink Maintained Repositories
******************************

Besides this repository (``tl_zephyr``), the SDK pulls in the following Telink
maintained modules through its west manifest (``west.yml``). Forked repositories
carry Telink-specific modifications and optimizations on top of upstream
projects; others are Telink provided components. All forked modules are
published under the ``tl_`` prefix to distinguish them from their upstreams:

- `tl_openthread`_ — fork of `OpenThread`_ with Telink security and performance patches
- `tl_xz`_ — fork of `XZ Utils`_ integrated as a Zephyr module
- `tl_openthread_libs`_ — Telink-provided prebuilt OpenThread FTD libraries
- `hal_telink`_ — Telink HAL module (follows the upstream ``hal_<vendor>`` naming convention)

For details on each module, see the linked repositories.

The modules above were previously published under their upstream-derived names
(``zephyr``, ``openthread``, ``xz`` and ``openthread_telink_lib``)
and have been renamed with the ``tl_`` prefix to make the Telink maintained
components of the SDK easy to identify.

.. _tl_openthread: https://github.com/telink-semi/tl_openthread
.. _OpenThread: https://github.com/openthread/openthread
.. _tl_xz: https://github.com/telink-semi/tl_xz
.. _XZ Utils: https://github.com/tukaani-project/xz
.. _tl_openthread_libs: https://github.com/telink-semi/tl_openthread_libs
.. _hal_telink: https://github.com/telink-semi/hal_telink

.. start_include_here

Community Support
*****************

Community support is provided via mailing lists and Discord; see the Resources
below for details.

.. _project-resources:

Resources
*********

Here's a quick summary of resources to help you find your way around:

* **Help**: `Asking for Help Tips`_
* **Documentation**: http://docs.zephyrproject.org (`Getting Started Guide`_)
* **Source Code**: https://github.com/zephyrproject-rtos/zephyr is the main
  repository; https://elixir.bootlin.com/zephyr/latest/source contains a
  searchable index
* **Releases**: https://github.com/zephyrproject-rtos/zephyr/releases
* **Samples and example code**: see `Sample and Demo Code Examples`_
* **Mailing Lists**: users@lists.zephyrproject.org and
  devel@lists.zephyrproject.org are the main user and developer mailing lists,
  respectively. You can join the developer's list and search its archives at
  `Zephyr Development mailing list`_. The other `Zephyr mailing list
  subgroups`_ have their own archives and sign-up pages.
* **Nightly CI Build Status**: https://lists.zephyrproject.org/g/builds
  The builds@lists.zephyrproject.org mailing list archives the CI nightly build results.
* **Chat**: Real-time chat happens in Zephyr's Discord Server. Use
  this `Discord Invite`_ to register.
* **Contributing**: see the `Contribution Guide`_
* **Wiki**: `Zephyr GitHub wiki`_
* **Issues**: https://github.com/zephyrproject-rtos/zephyr/issues
* **Security Issues**: Email vulnerabilities@zephyrproject.org to report
  security issues; also see our `Security`_ documentation. Security issues are
  tracked separately at https://zephyrprojectsec.atlassian.net.
* **Zephyr Project Website**: https://zephyrproject.org

.. _Discord Invite: https://chat.zephyrproject.org
.. _supported boards: http://docs.zephyrproject.org/latest/boards/index.html
.. _Zephyr Documentation: http://docs.zephyrproject.org
.. _Introduction to Zephyr: http://docs.zephyrproject.org/latest/introduction/index.html
.. _Getting Started Guide: http://docs.zephyrproject.org/latest/develop/getting_started/index.html
.. _Contribution Guide: http://docs.zephyrproject.org/latest/contribute/index.html
.. _Zephyr GitHub wiki: https://github.com/zephyrproject-rtos/zephyr/wiki
.. _Zephyr Development mailing list: https://lists.zephyrproject.org/g/devel
.. _Zephyr mailing list subgroups: https://lists.zephyrproject.org/g/main/subgroups
.. _Sample and Demo Code Examples: http://docs.zephyrproject.org/latest/samples/index.html
.. _Security: http://docs.zephyrproject.org/latest/security/index.html
.. _Asking for Help Tips: https://docs.zephyrproject.org/latest/develop/getting_started/index.html#asking-for-help

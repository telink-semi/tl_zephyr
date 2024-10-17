.. _timer_sample:

TIMER Usage Sample
##################

Overview
********

This sample demonstrates how to use the TIMERs in different modes.
The setup involves connecting interrupts triggered by timer events to
specific handlers and toggling GPIO pins (representing actions or indicators like LEDs).

Here is a brief description of the different modes and main functionalities
of the application:

Timer Modes
===========

The application supports multiple timer modes:

- **GPIO Trigger Mode**:
	Configures timers to trigger interrupts based on GPIO events.
	The interrupt handler toggles specified GPIO pins and counts
	interrupt occurrences.

- **GPIO Width Mode**:
	Measures pulse width using timers, capturing the duration of a
	high or low state on a GPIO pin.

- **Tick Mode**:
	Uses the timer to measure time in terms of clock ticks and can
	trigger periodic events based on elapsed time.

Interrupt Handling
==================

Each timer (Timer0 and Timer1) is associated with an interrupt
service routine (ISR) that processes the interrupt by clearing
the interrupt flag, performing mode-specific actions, and
toggling associated GPIOs or LEDs.

GPIO and Timer Initialization
=============================

- The application initializes several GPIOs for use as triggers,
	capture inputs, and indicators (LEDs).
- Sets up the necessary interrupts for each timer and enables them
	through the PLIC (Platform-Level Interrupt Controller).

Main Loop
=========

Continuously loops, updating or printing timer-related measurements
(like IRQ count or pulse width) and toggling LEDs to provide visual
feedback based on the timer's state.

Building and Running
********************

Build and flash the sample as follows:

.. zephyr-app-commands::
	 :zephyr-app: /samples/boards/tlsr9x/timer/
	 :board: tlsr9528a, tlsr9518adk80d
	 :goals: build flash
	 :compact:

Sample Output
=============

.. code-block:: console

		TIMER App starts!
		timer0_gpio_width: 0
		timer1_gpio_width: 546

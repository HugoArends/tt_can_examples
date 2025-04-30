# TTCAN example projects

Time-triggered CAN example projects for FRDM-MCXN947 development board.

# Description

This project aims to implement the NEN-ISO 11898-4:2014. Road vehicles — Controller area network (CAN) — Part 4: Time-triggered communication.

# Prerequisites

In order to build and run the projects, make sure the following prerequisites are met:

- **Toolchain**: Download and install MCUXpresso-IDE or MCUXpresso for VSCode.
- **SDK Installation**: Download and install the MCUXpresso SDK_25_03_00_FRDM-MCXN947 Software Development Kit.
- **Hardware Setup**: Use two (or more) FRDM-MCXN947 development boards. Connect onboard the CAN transceivers as described in the file *example_board_readme.md*.

Information is logged via serial terminals. Detailed timing measurents can be observed by connecting a logic analyser to the debug pins of both boards (see gpio_debug.h for the pins).

# Disclaimer

This content is provided for education and demonstration purpose.


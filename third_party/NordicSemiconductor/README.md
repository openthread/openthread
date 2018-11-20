Description:
This folder consists of selected components from the nRF5 SDK, that are used
in process of building the OpenThread's nRF52840 platform.

Directory consists of following folders:
 - /cmsis        - Core Peripheral Access Layer Headers files
 - /dependencies - Dependencies for drivers and libraries from nrf5 SDK
 - /drivers      - Drivers for the nRF52840 that are used in the platform
 - /libraries    - Libraries for the nRF52840 chip
 - /nrfx         - Standalone drivers for peripherals present in Nordic SoCs (https://github.com/NordicSemiconductor/nrfx)
 - /segger_rtt   - Library for the RTT communication
 - /softdevice   - SoftDevice s140 headers

 The following changes comparing to the nRF5 SDK 15.2 have been incorporated:
- modified nrf_log_ctrl.h file in order to remove unused backend logging functions
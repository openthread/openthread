Description:
This folder consists of selected components from the nRF5 SDK, that are used
in process of building the OpenThread's nRF528xx platform.

Directory consists of following folders:
 - /cmsis        - Core Peripheral Access Layer Headers files
 - /dependencies - Dependencies for drivers and libraries from nRF5 SDK
 - /drivers      - Drivers for the nRF528xx platform
 - /libraries    - Libraries for the nRF528xx platform
 - /nrfx         - Standalone drivers for peripherals present in Nordic SoCs (https://github.com/NordicSemiconductor/nrfx)
 - /segger_rtt   - Configuration files for RTT communication
 - /softdevice   - SoftDevice s140 headers

 The following changes comparing to the nRF5 SDK 16.0 have been incorporated:
- modified nrf_log module files in order to remove unused backend logging functions
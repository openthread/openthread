#ifndef SDK_CONFIG_H
#define SDK_CONFIG_H

#include <openthread/config.h>
#undef PACKAGE

#if defined(__GNUC__)
#pragma GCC diagnostic ignored "-Wpedantic"
#pragma GCC diagnostic ignored "-Wunused-parameter"
#endif

//==========================================================
// <e> APP_USBD_ENABLED - app_usbd - USB Device library
//==========================================================
#if (USB_CDC_AS_SERIAL_TRANSPORT == 1)
#ifndef APP_USBD_ENABLED
#define APP_USBD_ENABLED 1
#endif
#else  // USB_CDC_AS_SERIAL_TRANSPORT == 1
#ifndef APP_USBD_ENABLED
#define APP_USBD_ENABLED 0
#endif
#endif // USB_CDC_AS_SERIAL_TRANSPORT == 1

// <s> APP_USBD_VID - Vendor ID

// <i> Vendor ID ordered from USB IF: http://www.usb.org/developers/vendor/
#ifndef APP_USBD_VID
#define APP_USBD_VID 0x1915
#endif

// <s> APP_USBD_PID - Product ID

// <i> Selected Product ID
#ifndef APP_USBD_PID
#define APP_USBD_PID 0xCAFE
#endif

// <o> APP_USBD_DEVICE_VER_MAJOR - Device version, major part  <0-99>


// <i> Device version, will be converted automatically to BCD notation. Use just decimal values.

#ifndef APP_USBD_DEVICE_VER_MAJOR
#define APP_USBD_DEVICE_VER_MAJOR 1
#endif

// <o> APP_USBD_DEVICE_VER_MINOR - Device version, minor part  <0-99>


// <i> Device version, will be converted automatically to BCD notation. Use just decimal values.

#ifndef APP_USBD_DEVICE_VER_MINOR
#define APP_USBD_DEVICE_VER_MINOR 0
#endif

// <e> APP_USBD_EVENT_QUEUE_ENABLE - Enable event queue

// <i> This is the default configuration when all the events are placed into internal queue.
// <i> Disable it when external queue is used like app_scheduler or if you wish to process all events inside interrupts.
// <i> Processing all events from the interrupt level adds requirement not to call any functions that modifies the USBD library state from the context higher than USB interrupt context.
// <i> Functions that modify USBD state are functions for sleep, wakeup, start, stop, enable and disable.
//==========================================================
#ifndef APP_USBD_EVENT_QUEUE_ENABLE
#define APP_USBD_EVENT_QUEUE_ENABLE 1
#endif
// <o> APP_USBD_EVENT_QUEUE_SIZE - The size of event queue  <16-64>


// <i> The size of the queue for the events that would be processed in the main loop.

#ifndef APP_USBD_EVENT_QUEUE_SIZE
#define APP_USBD_EVENT_QUEUE_SIZE 32
#endif

// </e>

// <e> APP_USBD_CONFIG_LOG_ENABLED - Enable logging in the module
//==========================================================
#ifndef APP_USBD_CONFIG_LOG_ENABLED
#define APP_USBD_CONFIG_LOG_ENABLED 0
#endif

// </e>

// </e>

// <e> CLOCK_ENABLED - nrf_drv_clock - CLOCK peripheral driver
//==========================================================
#ifndef CLOCK_ENABLED
#define CLOCK_ENABLED 1
#endif
// <o> CLOCK_CONFIG_XTAL_FREQ  - HF XTAL Frequency

// <0=> Default (64 MHz)

#ifndef CLOCK_CONFIG_XTAL_FREQ
#define CLOCK_CONFIG_XTAL_FREQ 0
#endif

// <o> CLOCK_CONFIG_LF_SRC  - LF Clock Source

// <0=> RC
// <1=> XTAL
// <2=> Synth

#ifndef CLOCK_CONFIG_LF_SRC
#define CLOCK_CONFIG_LF_SRC 1
#endif

// <o> CLOCK_CONFIG_IRQ_PRIORITY  - Interrupt priority


// <i> Priorities 0,2 (nRF51) and 0,1,4,5 (nRF52) are reserved for SoftDevice
// <0=> 0 (highest)
// <1=> 1
// <2=> 2
// <3=> 3
// <4=> 4
// <5=> 5
// <6=> 6
// <7=> 7

#ifndef CLOCK_CONFIG_IRQ_PRIORITY
#define CLOCK_CONFIG_IRQ_PRIORITY 7
#endif

// </e>

// <e> POWER_ENABLED - nrf_drv_power - POWER peripheral driver
//==========================================================

#if (USB_CDC_AS_SERIAL_TRANSPORT == 1)
#ifndef POWER_ENABLED
#define POWER_ENABLED 1
#endif
#else  // USB_CDC_AS_SERIAL_TRANSPORT == 1
#ifndef POWER_ENABLED
#define POWER_ENABLED 0
#endif
#endif // USB_CDC_AS_SERIAL_TRANSPORT == 1

// <o> POWER_CONFIG_IRQ_PRIORITY  - Interrupt priority


// <i> Priorities 0,2 (nRF51) and 0,1,4,5 (nRF52) are reserved for SoftDevice
// <0=> 0 (highest)
// <1=> 1
// <2=> 2
// <3=> 3
// <4=> 4
// <5=> 5
// <6=> 6
// <7=> 7

#ifndef POWER_CONFIG_IRQ_PRIORITY
#define POWER_CONFIG_IRQ_PRIORITY 7
#endif

// <q> POWER_CONFIG_DEFAULT_DCDCEN  - The default configuration of main DCDC regulator


// <i> This settings means only that components for DCDC regulator are installed and it can be enabled.

#ifndef POWER_CONFIG_DEFAULT_DCDCEN
#define POWER_CONFIG_DEFAULT_DCDCEN 0
#endif

// <q> POWER_CONFIG_DEFAULT_DCDCENHV  - The default configuration of High Voltage DCDC regulator


// <i> This settings means only that components for DCDC regulator are installed and it can be enabled.

#ifndef POWER_CONFIG_DEFAULT_DCDCENHV
#define POWER_CONFIG_DEFAULT_DCDCENHV 0
#endif

// </e>

// <q> SYSTICK_ENABLED  - nrf_drv_systick - SysTick driver

#if (USB_CDC_AS_SERIAL_TRANSPORT == 1)
#ifndef SYSTICK_ENABLED
#define SYSTICK_ENABLED 1
#endif
#else  // USB_CDC_AS_SERIAL_TRANSPORT == 1
#ifndef SYSTICK_ENABLED
#define SYSTICK_ENABLED 0
#endif
#endif // USB_CDC_AS_SERIAL_TRANSPORT == 1

// <e> USBD_ENABLED - nrf_drv_usbd - USB driver
//==========================================================
#if (USB_CDC_AS_SERIAL_TRANSPORT == 1)
#ifndef USBD_ENABLED
#define USBD_ENABLED 1
#endif
#else  // USB_CDC_AS_SERIAL_TRANSPORT == 1
#ifndef USBD_ENABLED
#define USBD_ENABLED 0
#endif
#endif // USB_CDC_AS_SERIAL_TRANSPORT == 1
// <o> USBD_CONFIG_IRQ_PRIORITY  - Interrupt priority


// <i> Priorities 0,2 (nRF51) and 0,1,4,5 (nRF52) are reserved for SoftDevice
// <0=> 0 (highest)
// <1=> 1
// <2=> 2
// <3=> 3
// <4=> 4
// <5=> 5
// <6=> 6
// <7=> 7

#ifndef USBD_CONFIG_IRQ_PRIORITY
#define USBD_CONFIG_IRQ_PRIORITY 7
#endif

// <o> NRF_DRV_USBD_DMASCHEDULER_MODE  - USBD SMA scheduler working scheme

// <0=> Prioritized access
// <1=> Round Robin

#ifndef NRF_DRV_USBD_DMASCHEDULER_MODE
#define NRF_DRV_USBD_DMASCHEDULER_MODE 0
#endif

// </e>

// <h> app_usbd_cdc_acm - USB CDC ACM class

//==========================================================
// <q> APP_USBD_CLASS_CDC_ACM_ENABLED  - Enabling USBD CDC ACM Class library


#ifndef APP_USBD_CLASS_CDC_ACM_ENABLED
#define APP_USBD_CLASS_CDC_ACM_ENABLED 1
#endif

// </h>
//==========================================================

// <h> nrf_log - Logging

//==========================================================
// <e> NRF_LOG_ENABLED - Logging module for nRF5 SDK
//==========================================================
#ifndef NRF_LOG_ENABLED
#define NRF_LOG_ENABLED 0
#endif

// </e>
// </h>

#endif //SDK_CONFIG_H

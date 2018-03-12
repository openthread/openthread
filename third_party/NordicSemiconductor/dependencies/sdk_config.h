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

// <q> APP_USBD_CONFIG_SELF_POWERED  - Self powered


#ifndef APP_USBD_CONFIG_SELF_POWERED
#define APP_USBD_CONFIG_SELF_POWERED 1
#endif

// <q> APP_USBD_CONFIG_POWER_EVENTS_PROCESS  - Process power events


// <i> Enable processing power events in USB event handler.

#ifndef APP_USBD_CONFIG_POWER_EVENTS_PROCESS
#define APP_USBD_CONFIG_POWER_EVENTS_PROCESS 1
#endif

// <e> APP_USBD_CONFIG_EVENT_QUEUE_ENABLE - Enable event queue

// <i> This is the default configuration when all the events are placed into internal queue.
// <i> Disable it when external queue is used like app_scheduler or if you wish to process all events inside interrupts.
// <i> Processing all events from the interrupt level adds requirement not to call any functions that modifies the USBD library state from the context higher than USB interrupt context.
// <i> Functions that modify USBD state are functions for sleep, wakeup, start, stop, enable and disable.
//==========================================================
#ifndef APP_USBD_CONFIG_EVENT_QUEUE_ENABLE
#define APP_USBD_CONFIG_EVENT_QUEUE_ENABLE 1
#endif
// <o> APP_USBD_CONFIG_EVENT_QUEUE_SIZE - The size of event queue  <16-64>


// <i> The size of the queue for the events that would be processed in the main loop.

#ifndef APP_USBD_CONFIG_EVENT_QUEUE_SIZE
#define APP_USBD_CONFIG_EVENT_QUEUE_SIZE 32
#endif

// <o> APP_USBD_CONFIG_SOF_HANDLING_MODE  - Change SOF events handling mode.


// <i> Normal queue   - SOF events are pushed normally into event queue.
// <i> Compress queue - SOF events are counted and binded with other events or executed when queue is empty.
// <i>                  This prevents queue from filling with SOF events.
// <i> Interrupt      - SOF events are processed in interrupt.
// <0=> Normal queue
// <1=> Compress queue
// <2=> Interrupt

#ifndef APP_USBD_CONFIG_SOF_HANDLING_MODE
#define APP_USBD_CONFIG_SOF_HANDLING_MODE 1
#endif

// </e>

// <q> APP_USBD_CONFIG_SOF_TIMESTAMP_PROVIDE  - Provide a function that generates timestamps for logs based on the current SOF


// <i> The function app_usbd_sof_timestamp_get will be implemented if the logger is enabled.
// <i> Use it when initializing the logger.
// <i> SOF processing will be always enabled when this configuration parameter is active.
// <i> Notice that this option is configured outside of APP_USBD_CONFIG_LOG_ENABLED.
// <i> This means that it will work even if the logging in this very module is disabled.

#ifndef APP_USBD_CONFIG_SOF_TIMESTAMP_PROVIDE
#define APP_USBD_CONFIG_SOF_TIMESTAMP_PROVIDE 0
#endif

// <e> APP_USBD_CONFIG_LOG_ENABLED - Enable logging in the module
//==========================================================
#ifndef APP_USBD_CONFIG_LOG_ENABLED
#define APP_USBD_CONFIG_LOG_ENABLED 0
#endif

// </e>

// </e>

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

// <o> USBD_CONFIG_DMASCHEDULER_MODE  - USBD SMA scheduler working scheme

// <0=> Prioritized access
// <1=> Round Robin

#ifndef USBD_CONFIG_DMASCHEDULER_MODE
#define USBD_CONFIG_DMASCHEDULER_MODE 0
#endif

// </e>

// <h> app_usbd_cdc_acm - USB CDC ACM class

//==========================================================
// <q> APP_USBD_CDC_ACM_ENABLED  - app_usbd_cdc_acm - USB CDC ACM class


#ifndef APP_USBD_CDC_ACM_ENABLED
#define APP_USBD_CDC_ACM_ENABLED 1
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

// <h> segger_rtt - SEGGER RTT

//==========================================================
// <o> SEGGER_RTT_CONFIG_BUFFER_SIZE_UP - Size of upstream buffer.
// <i> Note that either @ref NRF_LOG_BACKEND_RTT_OUTPUT_BUFFER_SIZE
// <i> or this value is actually used. It depends on which one is bigger.

#ifndef SEGGER_RTT_CONFIG_BUFFER_SIZE_UP
#define SEGGER_RTT_CONFIG_BUFFER_SIZE_UP 512
#endif

// <o> SEGGER_RTT_CONFIG_MAX_NUM_UP_BUFFERS - Size of upstream buffer.
#ifndef SEGGER_RTT_CONFIG_MAX_NUM_UP_BUFFERS
#define SEGGER_RTT_CONFIG_MAX_NUM_UP_BUFFERS 2
#endif

// <o> SEGGER_RTT_CONFIG_BUFFER_SIZE_DOWN - Size of upstream buffer.
#ifndef SEGGER_RTT_CONFIG_BUFFER_SIZE_DOWN
#define SEGGER_RTT_CONFIG_BUFFER_SIZE_DOWN 16
#endif

// <o> SEGGER_RTT_CONFIG_MAX_NUM_DOWN_BUFFERS - Size of upstream buffer.
#ifndef SEGGER_RTT_CONFIG_MAX_NUM_DOWN_BUFFERS
#define SEGGER_RTT_CONFIG_MAX_NUM_DOWN_BUFFERS 2
#endif

// <o> SEGGER_RTT_CONFIG_DEFAULT_MODE  - RTT behavior if the buffer is full.


// <i> The following modes are supported:
// <i> - SKIP  - Do not block, output nothing.
// <i> - TRIM  - Do not block, output as much as fits.
// <i> - BLOCK - Wait until there is space in the buffer.
// <0=> SKIP
// <1=> TRIM
// <2=> BLOCK_IF_FIFO_FULL

#ifndef SEGGER_RTT_CONFIG_DEFAULT_MODE
#define SEGGER_RTT_CONFIG_DEFAULT_MODE 0
#endif

// </h>
//==========================================================

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

#endif //SDK_CONFIG_H

/**
 *
 * @defgroup nrfx_twim_config TWIM peripheral driver configuration
 * @{
 * @ingroup nrfx_twim
 */
/** @brief 
 *
 *  Set to 1 to activate.
 *
 * @note This is an NRF_CONFIG macro.
 */
#define NRFX_TWIM_ENABLED
/** @brief Enable TWIM0 instance
 *
 *  Set to 1 to activate.
 *
 * @note This is an NRF_CONFIG macro.
 */
#define NRFX_TWIM0_ENABLED

/** @brief Enable TWIM1 instance
 *
 *  Set to 1 to activate.
 *
 * @note This is an NRF_CONFIG macro.
 */
#define NRFX_TWIM1_ENABLED

/** @brief Frequency
 *
 *  Following options are available:
 * - 26738688 - 100k
 * - 67108864 - 250k
 * - 104857600 - 400k
 *
 * @note This is an NRF_CONFIG macro.
 */
#define NRFX_TWIM_DEFAULT_CONFIG_FREQUENCY

/** @brief Enables bus holding after uninit
 *
 *  Set to 1 to activate.
 *
 * @note This is an NRF_CONFIG macro.
 */
#define NRFX_TWIM_DEFAULT_CONFIG_HOLD_BUS_UNINIT

/** @brief Interrupt priority
 *
 *  Following options are available:
 * - 0 - 0 (highest)
 * - 1 - 1
 * - 2 - 2
 * - 3 - 3
 * - 4 - 4
 * - 5 - 5
 * - 6 - 6
 * - 7 - 7
 *
 * @note This is an NRF_CONFIG macro.
 */
#define NRFX_TWIM_DEFAULT_CONFIG_IRQ_PRIORITY

/** @brief Enables logging in the module.
 *
 *  Set to 1 to activate.
 *
 * @note This is an NRF_CONFIG macro.
 */
#define NRFX_TWIM_CONFIG_LOG_ENABLED
/** @brief Default Severity level
 *
 *  Following options are available:
 * - 0 - Off
 * - 1 - Error
 * - 2 - Warning
 * - 3 - Info
 * - 4 - Debug
 *
 * @note This is an NRF_CONFIG macro.
 */
#define NRFX_TWIM_CONFIG_LOG_LEVEL

/** @brief ANSI escape code prefix.
 *
 *  Following options are available:
 * - 0 - Default
 * - 1 - Black
 * - 2 - Red
 * - 3 - Green
 * - 4 - Yellow
 * - 5 - Blue
 * - 6 - Magenta
 * - 7 - Cyan
 * - 8 - White
 *
 * @note This is an NRF_CONFIG macro.
 */
#define NRFX_TWIM_CONFIG_INFO_COLOR

/** @brief ANSI escape code prefix.
 *
 *  Following options are available:
 * - 0 - Default
 * - 1 - Black
 * - 2 - Red
 * - 3 - Green
 * - 4 - Yellow
 * - 5 - Blue
 * - 6 - Magenta
 * - 7 - Cyan
 * - 8 - White
 *
 * @note This is an NRF_CONFIG macro.
 */
#define NRFX_TWIM_CONFIG_DEBUG_COLOR


/** @brief Enables nRF52 anomaly 109 workaround for TWIM.
 *
 * The workaround uses interrupts to wake up the CPU by catching
 * the start event of zero-frequency transmission, clear the 
 * peripheral, set desired frequency, start the peripheral, and
 * the proper transmission. See more in the Errata document or
 * Anomaly 109 Addendum located at https://infocenter.nordicsemi.com/
 *
 *  Set to 1 to activate.
 *
 * @note This is an NRF_CONFIG macro.
 */
#define NRFX_TWIM_NRF52_ANOMALY_109_WORKAROUND_ENABLED


/** @} */

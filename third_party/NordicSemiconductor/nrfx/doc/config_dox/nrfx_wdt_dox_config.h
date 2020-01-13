/**
 *
 * @defgroup nrfx_wdt_config WDT peripheral driver configuration
 * @{
 * @ingroup nrfx_wdt
 */
/** @brief Enable WDT driver
 *
 *  Set to 1 to activate.
 *
 * @note This is an NRF_CONFIG macro.
 */
#define NRFX_WDT_ENABLED
/** @brief WDT behavior in CPU SLEEP or HALT mode
 *
 *  Following options are available:
 * - 1 - Run in SLEEP, Pause in HALT
 * - 8 - Pause in SLEEP, Run in HALT
 * - 9 - Run in SLEEP and HALT
 * - 0 - Pause in SLEEP and HALT
 *
 * @note This is an NRF_CONFIG macro.
 */
#define NRFX_WDT_CONFIG_BEHAVIOUR

/** @brief Reload value
 *
 *  Minimum value: 15
 *  Maximum value: 4294967295
 *
 * @note This is an NRF_CONFIG macro.
 */
#define NRFX_WDT_CONFIG_RELOAD_VALUE

/** @brief Remove WDT IRQ handling from WDT driver
 *
 *  Following options are available:
 * - 0 - Include WDT IRQ handling
 * - 1 - Remove WDT IRQ handling
 *
 * @note This is an NRF_CONFIG macro.
 */
#define NRFX_WDT_CONFIG_NO_IRQ

/** @brief Interrupt priority
 *
 *  Following options are available:
 * - 0 - 0 (highest)
 * - 1 - 1
 * - 2 - 2
 * - 3 - 3
 * - 4 - 4 (Not applicable for nRF51)
 * - 5 - 5 (Not applicable for nRF51)
 * - 6 - 6 (Not applicable for nRF51)
 * - 7 - 7 (Not applicable for nRF51)
 *
 * @note This is an NRF_CONFIG macro.
 */
#define NRFX_WDT_CONFIG_IRQ_PRIORITY

/** @brief Enables logging in the module.
 *
 *  Set to 1 to activate.
 *
 * @note This is an NRF_CONFIG macro.
 */
#define NRFX_WDT_CONFIG_LOG_ENABLED
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
#define NRFX_WDT_CONFIG_LOG_LEVEL

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
#define NRFX_WDT_CONFIG_INFO_COLOR

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
#define NRFX_WDT_CONFIG_DEBUG_COLOR



/** @} */

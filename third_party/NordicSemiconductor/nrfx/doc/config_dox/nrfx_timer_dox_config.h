/**
 *
 * @defgroup nrfx_timer_config TIMER periperal driver configuration
 * @{
 * @ingroup nrfx_timer
 */
/** @brief 
 *
 *  Set to 1 to activate.
 *
 * @note This is an NRF_CONFIG macro.
 */
#define NRFX_TIMER_ENABLED
/** @brief Enable TIMER0 instance
 *
 *  Set to 1 to activate.
 *
 * @note This is an NRF_CONFIG macro.
 */
#define NRFX_TIMER0_ENABLED

/** @brief Enable TIMER1 instance
 *
 *  Set to 1 to activate.
 *
 * @note This is an NRF_CONFIG macro.
 */
#define NRFX_TIMER1_ENABLED

/** @brief Enable TIMER2 instance
 *
 *  Set to 1 to activate.
 *
 * @note This is an NRF_CONFIG macro.
 */
#define NRFX_TIMER2_ENABLED

/** @brief Enable TIMER3 instance
 *
 *  Set to 1 to activate.
 *
 * @note This is an NRF_CONFIG macro.
 */
#define NRFX_TIMER3_ENABLED

/** @brief Enable TIMER4 instance
 *
 *  Set to 1 to activate.
 *
 * @note This is an NRF_CONFIG macro.
 */
#define NRFX_TIMER4_ENABLED

/** @brief Timer frequency if in Timer mode
 *
 *  Following options are available:
 * - 0 - 16 MHz
 * - 1 - 8 MHz
 * - 2 - 4 MHz
 * - 3 - 2 MHz
 * - 4 - 1 MHz
 * - 5 - 500 kHz
 * - 6 - 250 kHz
 * - 7 - 125 kHz
 * - 8 - 62.5 kHz
 * - 9 - 31.25 kHz
 *
 * @note This is an NRF_CONFIG macro.
 */
#define NRFX_TIMER_DEFAULT_CONFIG_FREQUENCY

/** @brief Timer mode or operation
 *
 *  Following options are available:
 * - 0 - Timer
 * - 1 - Counter
 *
 * @note This is an NRF_CONFIG macro.
 */
#define NRFX_TIMER_DEFAULT_CONFIG_MODE

/** @brief Timer counter bit width
 *
 *  Following options are available:
 * - 0 - 16 bit
 * - 1 - 8 bit
 * - 2 - 24 bit
 * - 3 - 32 bit
 *
 * @note This is an NRF_CONFIG macro.
 */
#define NRFX_TIMER_DEFAULT_CONFIG_BIT_WIDTH

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
#define NRFX_TIMER_DEFAULT_CONFIG_IRQ_PRIORITY

/** @brief Enables logging in the module.
 *
 *  Set to 1 to activate.
 *
 * @note This is an NRF_CONFIG macro.
 */
#define NRFX_TIMER_CONFIG_LOG_ENABLED
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
#define NRFX_TIMER_CONFIG_LOG_LEVEL

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
#define NRFX_TIMER_CONFIG_INFO_COLOR

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
#define NRFX_TIMER_CONFIG_DEBUG_COLOR



/** @} */

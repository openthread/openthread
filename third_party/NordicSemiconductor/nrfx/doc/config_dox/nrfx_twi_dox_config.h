/**
 *
 * @defgroup nrfx_twi_config TWI peripheral driver configuration
 * @{
 * @ingroup nrfx_twi
 */
/** @brief 
 *
 *  Set to 1 to activate.
 *
 * @note This is an NRF_CONFIG macro.
 */
#define NRFX_TWI_ENABLED
/** @brief Enable TWI0 instance
 *
 *  Set to 1 to activate.
 *
 * @note This is an NRF_CONFIG macro.
 */
#define NRFX_TWI0_ENABLED

/** @brief Enable TWI1 instance
 *
 *  Set to 1 to activate.
 *
 * @note This is an NRF_CONFIG macro.
 */
#define NRFX_TWI1_ENABLED

/** @brief Frequency
 *
 *  Following options are available:
 * - 26738688 - 100k
 * - 67108864 - 250k
 * - 104857600 - 400k
 *
 * @note This is an NRF_CONFIG macro.
 */
#define NRFX_TWI_DEFAULT_CONFIG_FREQUENCY

/** @brief Enables bus holding after uninit
 *
 *  Set to 1 to activate.
 *
 * @note This is an NRF_CONFIG macro.
 */
#define NRFX_TWI_DEFAULT_CONFIG_HOLD_BUS_UNINIT

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
#define NRFX_TWI_DEFAULT_CONFIG_IRQ_PRIORITY

/** @brief Enables logging in the module.
 *
 *  Set to 1 to activate.
 *
 * @note This is an NRF_CONFIG macro.
 */
#define NRFX_TWI_CONFIG_LOG_ENABLED
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
#define NRFX_TWI_CONFIG_LOG_LEVEL

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
#define NRFX_TWI_CONFIG_INFO_COLOR

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
#define NRFX_TWI_CONFIG_DEBUG_COLOR



/** @} */

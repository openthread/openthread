/**
 *
 * @defgroup nrfx_power_config POWER peripheral driver configuration
 * @{
 * @ingroup nrfx_power
 */
/** @brief Enable POWER driver
 *
 *  Set to 1 to activate.
 *
 * @note This is an NRF_CONFIG macro.
 */
#define NRFX_POWER_ENABLED
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
#define NRFX_POWER_CONFIG_IRQ_PRIORITY

/** @brief The default configuration of main DCDC regulator
 *
 * This settings means only that components for DCDC regulator are installed and it can be enabled.
 *
 *  Set to 1 to activate.
 *
 * @note This is an NRF_CONFIG macro.
 */
#define NRFX_POWER_CONFIG_DEFAULT_DCDCEN

/** @brief The default configuration of High Voltage DCDC regulator
 *
 * This settings means only that components for DCDC regulator are installed and it can be enabled.
 *
 *  Set to 1 to activate.
 *
 * @note This is an NRF_CONFIG macro.
 */
#define NRFX_POWER_CONFIG_DEFAULT_DCDCENHV


/** @} */

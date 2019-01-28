/**
 *
 * @defgroup nrfx_lpcomp_config LPCOMP peripheral driver configuration
 * @{
 * @ingroup nrfx_lpcomp
 */
/** @brief Enable LPCOMP driver
 *
 *  Set to 1 to activate.
 *
 * @note This is an NRF_CONFIG macro.
 */
#define NRFX_LPCOMP_ENABLED
/** @brief Reference voltage
 *
 *  Following options are available:
 * - 0 - Supply 1/8
 * - 1 - Supply 2/8
 * - 2 - Supply 3/8
 * - 3 - Supply 4/8
 * - 4 - Supply 5/8
 * - 5 - Supply 6/8
 * - 6 - Supply 7/8
 * - 8 - Supply 1/16 (nRF52)
 * - 9 - Supply 3/16 (nRF52)
 * - 10 - Supply 5/16 (nRF52)
 * - 11 - Supply 7/16 (nRF52)
 * - 12 - Supply 9/16 (nRF52)
 * - 13 - Supply 11/16 (nRF52)
 * - 14 - Supply 13/16 (nRF52)
 * - 15 - Supply 15/16 (nRF52)
 * - 7 - External Ref 0
 * - 65543 - External Ref 1
 *
 * @note This is an NRF_CONFIG macro.
 */
#define NRFX_LPCOMP_CONFIG_REFERENCE

/** @brief Detection
 *
 *  Following options are available:
 * - 0 - Crossing
 * - 1 - Up
 * - 2 - Down
 *
 * @note This is an NRF_CONFIG macro.
 */
#define NRFX_LPCOMP_CONFIG_DETECTION

/** @brief Analog input
 *
 *  Following options are available:
 * - 0
 * - 1
 * - 2
 * - 3
 * - 4
 * - 5
 * - 6
 * - 7
 *
 * @note This is an NRF_CONFIG macro.
 */
#define NRFX_LPCOMP_CONFIG_INPUT

/** @brief Hysteresis
 *
 *  Set to 1 to activate.
 *
 * @note This is an NRF_CONFIG macro.
 */
#define NRFX_LPCOMP_CONFIG_HYST

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
#define NRFX_LPCOMP_CONFIG_IRQ_PRIORITY

/** @brief Enables logging in the module.
 *
 *  Set to 1 to activate.
 *
 * @note This is an NRF_CONFIG macro.
 */
#define NRFX_LPCOMP_CONFIG_LOG_ENABLED
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
#define NRFX_LPCOMP_CONFIG_LOG_LEVEL

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
#define NRFX_LPCOMP_CONFIG_INFO_COLOR

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
#define NRFX_LPCOMP_CONFIG_DEBUG_COLOR



/** @} */

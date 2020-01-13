/**
 *
 * @defgroup nrfx_saadc_config SAADC peripheral driver configuration
 * @{
 * @ingroup nrfx_saadc
 */
/** @brief Enable SAADC driver
 *
 *  Set to 1 to activate.
 *
 * @note This is an NRF_CONFIG macro.
 */
#define NRFX_SAADC_ENABLED
/** @brief Resolution
 *
 *  Following options are available:
 * - 0 - 8 bit
 * - 1 - 10 bit
 * - 2 - 12 bit
 * - 3 - 14 bit
 *
 * @note This is an NRF_CONFIG macro.
 */
#define NRFX_SAADC_CONFIG_RESOLUTION

/** @brief Sample period
 *
 *  Following options are available:
 * - 0 - Disabled
 * - 1 - 2x
 * - 2 - 4x
 * - 3 - 8x
 * - 4 - 16x
 * - 5 - 32x
 * - 6 - 64x
 * - 7 - 128x
 * - 8 - 256x
 *
 * @note This is an NRF_CONFIG macro.
 */
#define NRFX_SAADC_CONFIG_OVERSAMPLE

/** @brief Enabling low power mode
 *
 *  Set to 1 to activate.
 *
 * @note This is an NRF_CONFIG macro.
 */
#define NRFX_SAADC_CONFIG_LP_MODE

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
#define NRFX_SAADC_CONFIG_IRQ_PRIORITY

/** @brief Enables logging in the module.
 *
 *  Set to 1 to activate.
 *
 * @note This is an NRF_CONFIG macro.
 */
#define NRFX_SAADC_CONFIG_LOG_ENABLED
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
#define NRFX_SAADC_CONFIG_LOG_LEVEL

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
#define NRFX_SAADC_CONFIG_INFO_COLOR

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
#define NRFX_SAADC_CONFIG_DEBUG_COLOR



/** @} */

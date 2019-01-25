/**
 *
 * @defgroup nrfx_uarte_config UARTE peripheral driver configuration
 * @{
 * @ingroup nrfx_uarte
 */
/** @brief Enable UARTE driver
 *
 *  Set to 1 to activate.
 *
 * @note This is an NRF_CONFIG macro.
 */
#define NRFX_UARTE_ENABLED
/** @brief Enable UARTE0 instance
 *
 *
 * @note This is an NRF_CONFIG macro.
 */
#define NRFX_UARTE0_ENABLED

/** @brief Enable UARTE1 instance
 *
 *
 * @note This is an NRF_CONFIG macro.
 */
#define NRFX_UARTE1_ENABLED

/** @brief Hardware Flow Control
 *
 *  Following options are available:
 * - 0 - Disabled
 * - 1 - Enabled
 *
 * @note This is an NRF_CONFIG macro.
 */
#define NRFX_UARTE_DEFAULT_CONFIG_HWFC

/** @brief Parity
 *
 *  Following options are available:
 * - 0 - Excluded
 * - 14 - Included
 *
 * @note This is an NRF_CONFIG macro.
 */
#define NRFX_UARTE_DEFAULT_CONFIG_PARITY

/** @brief Default Baudrate
 *
 *  Following options are available:
 * - 323584 - 1200 baud
 * - 643072 - 2400 baud
 * - 1290240 - 4800 baud
 * - 2576384 - 9600 baud
 * - 3862528 - 14400 baud
 * - 5152768 - 19200 baud
 * - 7716864 - 28800 baud
 * - 8388608 - 31250 baud
 * - 10289152 - 38400 baud
 * - 15007744 - 56000 baud
 * - 15400960 - 57600 baud
 * - 20615168 - 76800 baud
 * - 30801920 - 115200 baud
 * - 61865984 - 230400 baud
 * - 67108864 - 250000 baud
 * - 121634816 - 460800 baud
 * - 251658240 - 921600 baud
 * - 268435456 - 1000000 baud
 *
 * @note This is an NRF_CONFIG macro.
 */
#define NRFX_UARTE_DEFAULT_CONFIG_BAUDRATE

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
#define NRFX_UARTE_DEFAULT_CONFIG_IRQ_PRIORITY

/** @brief Enables logging in the module.
 *
 *  Set to 1 to activate.
 *
 * @note This is an NRF_CONFIG macro.
 */
#define NRFX_UARTE_CONFIG_LOG_ENABLED
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
#define NRFX_UARTE_CONFIG_LOG_LEVEL

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
#define NRFX_UARTE_CONFIG_INFO_COLOR

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
#define NRFX_UARTE_CONFIG_DEBUG_COLOR



/** @} */
